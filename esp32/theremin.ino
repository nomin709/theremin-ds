// ============================================================
// Two-Hand Theremin for ESP32 - Phase 0 Firmware (30 Hz ML Stream)
// 
// Features:
// - Non-blocking deterministic 30 Hz loop (33.3 ms sampling)
// - Hardware-capped ultrasonic pulseIn timeout (6000 us / ~100 cm max range)
// - On-device real-time kinematics (Velocity & Acceleration calculation)
// - Fixed compact CSV serial payload stream for low-latency ML ingestion
// - Multi-button gestures: Single press (Calibrate), Double press (Default Vol), Extra Long (Power)
// - Dynamic state RGB LED indicators
//
// Serial CSV Format (30 Hz):
// RAW_DIST,SMOOTH_DIST,VEL,ACC,RAW_PHOTO,SMOOTH_PHOTO,NOTE,MODE
// Example: 23.4,22.1,1.25,0.30,2927,0.89,440.0,0
// ============================================================

#include <Arduino.h>
#include <math.h>

// ---------------- PIN DEFINITIONS ----------------
#define TRIG_PIN       5
#define ECHO_PIN       18
#define BUTTON_PIN     4
#define PHOTO_PIN      34
#define LED_R_PIN      25
#define LED_G_PIN      26
#define LED_B_PIN      27
#define SPEAKER_PIN    19

// ---------------- TIMING & SAMPLING ----------------
const unsigned long SAMPLE_INTERVAL_US = 33333; // 30 Hz target (33.333 ms)
unsigned long lastSampleTimeUs = 0;

// ---------------- SPEAKER PWM ----------------
const int SPEAKER_RES_BITS = 8;
const int SPEAKER_BASE_FREQ = 1000;

// ---------------- BUTTON SETTINGS ----------------
const unsigned long DEBOUNCE_MS = 40;
const unsigned long EXTRA_LONG_PRESS_MS = 3000;
const unsigned long DOUBLE_PRESS_MS = 350;

// ---------------- ULTRASONIC & KINEMATICS SETTINGS ----------------
const float MIN_DIST_CM = 5.0f;
const float MAX_DIST_CM = 60.0f;
const float DIST_SMOOTH_ALPHA = 0.25f;
const unsigned long ULTRASONIC_TIMEOUT_US = 6000; // Cap at ~100cm range to guard 30Hz loop

// ---------------- PHOTORESISTOR SETTINGS ----------------
const float VOL_SMOOTH_ALPHA = 0.15f;
const int ADC_MIN_DEFAULT = 800;
const int ADC_MAX_DEFAULT = 3200;

// ---------------- DEFAULT VOLUME MODE ----------------
bool useDefaultVolume = false;
const float DEFAULT_VOLUME = 0.40f;

// ---------------- CALIBRATION SETTINGS ----------------
const unsigned long CALIBRATION_MS = 5000;

// ---------------- FIXED NOTES ----------------
const float NOTE_SET[] = {261.63, 293.66, 329.63, 392.00, 440.00, 523.25, 587.33, 659.25};
const int NOTE_COUNT = 8;

// ---------------- SYSTEM STATE ----------------
bool powerOn = true;
bool calibrationMode = false;
bool isCalibrated = false;
int lightMin = ADC_MIN_DEFAULT;
int lightMax = ADC_MAX_DEFAULT;

// Sensor & Kinematics Variables
float smoothedDist = 20.0f;
float prevDist = 20.0f;
float currentVelocity = 0.0f; // cm/s
float prevVelocity = 0.0f;
float currentAccel = 0.0f;    // cm/s^2

float smoothedVol = 0.5f;
float currentPitch = 261.63f;
bool currentlyPlaying = false;

// Button state tracking
bool buttonStableState = HIGH;
bool lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long pressStartTime = 0;
bool extraLongPressHandled = false;

// Double press tracking
bool waitingForSecondPress = false;
unsigned long firstPressReleaseTime = 0;

// ---------------- MATH HELPERS ----------------
float clampf(float x, float lo, float hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

float mapFloat(float x, float inMin, float inMax, float outMin, float outMax) {
  if (fabs(inMax - inMin) < 1e-6) return outMin;
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

// ---------------- LED CONTROL ----------------
void setLEDColor(bool r, bool g, bool b) {
  digitalWrite(LED_R_PIN, r ? HIGH : LOW);
  digitalWrite(LED_G_PIN, g ? HIGH : LOW);
  digitalWrite(LED_B_PIN, b ? HIGH : LOW);
}

void turnLEDOff() {
  setLEDColor(false, false, false);
}

void updateDeviceLED() {
  if (!powerOn) {
    turnLEDOff();
    return;
  }

  if (calibrationMode) return;

  if (useDefaultVolume) {
    static unsigned long lastBlink = 0;
    static bool ledOn = true;
    const unsigned long BLINK_MS = 1000;

    if (millis() - lastBlink >= BLINK_MS) {
      lastBlink = millis();
      ledOn = !ledOn;
    }

    if (!ledOn) {
      turnLEDOff();
      return;
    }
  }

  setLEDColor(false, true, false); // Solid green when active
}

void blinkCalibrationLED() {
  static unsigned long lastBlink = 0;
  static bool state = false;
  const unsigned long BLINK_MS = 180;

  if (millis() - lastBlink >= BLINK_MS) {
    lastBlink = millis();
    state = !state;

    if (state) setLEDColor(true, false, false);   // Red
    else       setLEDColor(false, false, true);   // Blue
  }
}

// ---------------- ULTRASONIC SENSOR ----------------
float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Capped pulseIn timeout at 6000 microseconds (~100cm range) to enforce strict 30Hz timing
  long duration = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_US);

  if (duration == 0) return -1.0f;
  return (duration * 0.0343f) / 2.0f;
}

// ---------------- AUDIO SYNTHESIS ----------------
void stopSpeaker() {
  ledcWriteTone(SPEAKER_PIN, 0);
  ledcWrite(SPEAKER_PIN, 0);
}

void playSpeaker(float freqHz, float volume01) {
  volume01 = clampf(volume01, 0.0f, 1.0f);

  if (!powerOn) {
    stopSpeaker();
    return;
  }

  ledcWriteTone(SPEAKER_PIN, freqHz);

  int duty = (int)(volume01 * 110.0f);
  duty = constrain(duty, 0, 180);
  ledcWrite(SPEAKER_PIN, duty);
}

// ---------------- CALIBRATION ----------------
void runPhotoCalibration() {
  calibrationMode = true;
  stopSpeaker();
  Serial.println("CALIBRATION START");

  unsigned long start = millis();
  int minVal = 4095;
  int maxVal = 0;

  while (millis() - start < CALIBRATION_MS) {
    blinkCalibrationLED();

    int raw = analogRead(PHOTO_PIN);
    if (raw < minVal) minVal = raw;
    if (raw > maxVal) maxVal = raw;

    delay(10);
  }

  if (maxVal - minVal < 150) {
    lightMin = ADC_MIN_DEFAULT;
    lightMax = ADC_MAX_DEFAULT;
  } else {
    lightMin = minVal;
    lightMax = maxVal;
  }

  calibrationMode = false;
  updateDeviceLED();
  Serial.println("CALIBRATION DONE");
  isCalibrated = true;
}

// ---------------- BUTTON EVENT HANDLERS ----------------
void handleSinglePress() {
  if (!powerOn) return;
  runPhotoCalibration();
}

void handleDoublePress() {
  if (!powerOn) return;
  useDefaultVolume = !useDefaultVolume;
  updateDeviceLED();
}

void handleExtraLongPress() {
  powerOn = !powerOn;

  if (!powerOn) {
    stopSpeaker();
    currentlyPlaying = false;
    turnLEDOff();
    Serial.println("SYSTEM OFF");
  } else {
    updateDeviceLED();
    Serial.println("SYSTEM ON");
  }
}

void updateButton() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != buttonStableState) {
      buttonStableState = reading;

      if (buttonStableState == LOW) {
        pressStartTime = millis();
        extraLongPressHandled = false;
      } else {
        unsigned long pressDuration = millis() - pressStartTime;

        if (!extraLongPressHandled) {
          if (pressDuration < EXTRA_LONG_PRESS_MS) {
            if (waitingForSecondPress) {
              waitingForSecondPress = false;
              handleDoublePress();
            } else {
              waitingForSecondPress = true;
              firstPressReleaseTime = millis();
            }
          }
        }
      }
    }

    if (buttonStableState == LOW && !extraLongPressHandled) {
      unsigned long held = millis() - pressStartTime;
      if (held >= EXTRA_LONG_PRESS_MS) {
        extraLongPressHandled = true;
        waitingForSecondPress = false;
        handleExtraLongPress();
      }
    }
  }

  lastButtonReading = reading;

  if (waitingForSecondPress && (millis() - firstPressReleaseTime > DOUBLE_PRESS_MS)) {
    waitingForSecondPress = false;
    handleSinglePress();
  }
}

// ---------------- PITCH CONVERSION ----------------
float distanceToPitch(float distCM) {
  distCM = clampf(distCM, MIN_DIST_CM, MAX_DIST_CM);

  int idx = (int)roundf(mapFloat(distCM, MIN_DIST_CM, MAX_DIST_CM, 0.0f, (float)(NOTE_COUNT - 1)));
  idx = constrain(idx, 0, NOTE_COUNT - 1);

  return NOTE_SET[idx];
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);

  analogReadResolution(12);

  ledcAttach(SPEAKER_PIN, SPEAKER_BASE_FREQ, SPEAKER_RES_BITS);
  ledcWrite(SPEAKER_PIN, 0);

  updateDeviceLED();

  Serial.println("SYSTEM ON");
  lastSampleTimeUs = micros();
}

// ---------------- MAIN LOOP ----------------
void loop() {
  updateButton();

  if (!powerOn) {
    delay(30);
    return;
  }

  // Non-blocking deterministic tick timer (30 Hz / 33.3 ms)
  unsigned long currentUs = micros();
  if (currentUs - lastSampleTimeUs < SAMPLE_INTERVAL_US) {
    return; 
  }

  float dt = (currentUs - lastSampleTimeUs) / 1000000.0f; // Exact delta time in seconds
  lastSampleTimeUs = currentUs;

  // 1. Read Sensors
  float rawDist = readDistanceCM();
  bool activeHand = (rawDist >= MIN_DIST_CM && rawDist <= MAX_DIST_CM);

  int rawPhoto = analogRead(PHOTO_PIN);
  float vNorm = mapFloat((float)rawPhoto, (float)lightMin, (float)lightMax, 0.0f, 1.0f);
  vNorm = clampf(vNorm, 0.0f, 1.0f);

  // 2. Process Kinematics & Signal Smoothing
  if (activeHand) {
    smoothedDist = (1.0f - DIST_SMOOTH_ALPHA) * smoothedDist + DIST_SMOOTH_ALPHA * rawDist;
    
    // Calculate Velocity (cm/s) & Acceleration (cm/s^2)
    currentVelocity = (smoothedDist - prevDist) / dt;
    currentAccel = (currentVelocity - prevVelocity) / dt;

    prevDist = smoothedDist;
    prevVelocity = currentVelocity;

    currentPitch = distanceToPitch(smoothedDist);
  } else {
    // Reset/decay kinematics when hand is absent
    currentVelocity = 0.0f;
    currentAccel = 0.0f;
    prevVelocity = 0.0f;
  }

  smoothedVol = (1.0f - VOL_SMOOTH_ALPHA) * smoothedVol + VOL_SMOOTH_ALPHA * vNorm;
  float finalVol = useDefaultVolume ? DEFAULT_VOLUME : smoothedVol;

  currentlyPlaying = (!calibrationMode && activeHand);

  // 3. Audio & Visual Feedback
  if (currentlyPlaying) {
    playSpeaker(currentPitch, finalVol);
  } else {
    stopSpeaker();
  }

  if (!calibrationMode) {
    updateDeviceLED();
  }

  // 4. Compact CSV Serial Stream for Machine Learning Pipeline
  // Schema: RAW_DIST,SMOOTH_DIST,VEL,ACC,RAW_PHOTO,SMOOTH_PHOTO,NOTE,MODE
  float serialDist = activeHand ? smoothedDist : -1.0f;
  float serialRawDist = activeHand ? rawDist : -1.0f;
  int currentMode = useDefaultVolume ? 1 : 0;

  Serial.print(serialRawDist, 1);
  Serial.print(",");
  Serial.print(serialDist, 1);
  Serial.print(",");
  Serial.print(currentVelocity, 2);
  Serial.print(",");
  Serial.print(currentAccel, 2);
  Serial.print(",");
  Serial.print(rawPhoto);
  Serial.print(",");
  Serial.print(vNorm, 2);
  Serial.print(",");
  Serial.print(currentPitch, 1);
  Serial.print(",");
  Serial.println(currentMode);
}