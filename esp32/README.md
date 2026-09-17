# ESP32 Firmware — `theremin.ino`

The firmware runs a deterministic 30Hz control loop on the ESP32, reading two sensors, computing on-device kinematics, driving audio output, and streaming a structured CSV over serial for real-time visualization.

---

## Wiring

### HC-SR04 Ultrasonic Sensor

The HC-SR04 operates at 5V and its ECHO pin outputs 5V — above the ESP32's 3.3V GPIO limit. A voltage divider is required on the ECHO line.

```
HC-SR04 VCC  → 5V
HC-SR04 GND  → GND
HC-SR04 TRIG → GPIO 5

HC-SR04 ECHO → [1kΩ] → GPIO 18
                            |
                          [2kΩ]
                            |
                           GND
```

### Photoresistor

```
3.3V → [10kΩ] → GPIO 34
                     |
             [Photoresistor]
                     |
                    GND
```

Covered hand → high resistance → low ADC value → quiet
Uncovered hand → low resistance → high ADC value → loud

### RGB LED (Common Cathode)

```
GPIO 25 → [220Ω] → LED R pin
GPIO 26 → [220Ω] → LED G pin
GPIO 27 → [220Ω] → LED B pin
GND     → LED GND (common cathode)
```

### PAM8403 Amplifier + Speaker

```
GPIO 19  → PAM8403 L+ input
GND      → PAM8403 L− input
5V       → PAM8403 VCC
GND      → PAM8403 GND
PAM8403 LOUT+ → Speaker red wire
PAM8403 LOUT− → Speaker black wire
```

Use the V1 potentiometer on the PAM8403 to set hardware volume ceiling.

### Button

```
GPIO 4 → Button → GND
```

Uses `INPUT_PULLUP` — no external resistor needed.

---

## Pin Summary

| GPIO | Component | Direction |
|---|---|---|
| 5 | HC-SR04 TRIG | Output |
| 18 | HC-SR04 ECHO (via divider) | Input |
| 19 | PAM8403 audio input | Output |
| 25 | RGB LED Red | Output |
| 26 | RGB LED Green | Output |
| 27 | RGB LED Blue | Output |
| 34 | Photoresistor (analog) | Input |
| 4 | Button | Input (pullup) |

---

## How to Upload

### Board Setup

1. Install the ESP32 board package:
   - Arduino IDE → **File → Preferences**
   - Add to Additional Boards Manager URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
   - **Tools → Board → Boards Manager** → search `esp32` → install **esp32 by Espressif Systems**

2. Select your board:
   - **Tools → Board → ESP32 Arduino → ESP32 Dev Module**

3. Set upload speed:
   - **Tools → Upload Speed → 115200**

4. Select your port:
   - **Tools → Port** → choose your ESP32 port

### Upload

Click **Upload**. If it fails at `Connecting....`, hold the **BOOT** button on the ESP32 until `Uploading...` appears, then release.

---

## Design Decisions

| Decision | Reason |
|---|---|
| `micros()` timer instead of `delay()` | Non-blocking 30Hz loop — button and LED logic runs every iteration regardless of sensor timing |
| `pulseIn()` timeout capped at 6000μs | Prevents a missed ultrasonic echo from stalling the loop for up to 30ms |
| Exponential smoothing (EMA) over moving average | No buffer required, O(1) compute, alpha gives direct control over smoothing vs responsiveness |
| Exact `dt` for kinematics | Velocity and acceleration computed from measured elapsed time — accurate even under timing jitter |
| Calibration fallback (<150 ADC range) | If user does nothing during calibration, system falls back to safe defaults instead of locking into a bad range |
| LEDC PWM for audio | ESP32 core v3.x API — volume mapped to duty cycle for clean square wave output via PAM8403 |

---

## Button Gestures

A single button handles three distinct interactions via a non-blocking state machine:

| Gesture | Timing | Action |
|---|---|---|
| Single press | Short press, released within 350ms of first | Run 5-second photoresistor calibration |
| Double press | Two presses within 350ms of each other | Toggle default volume mode |
| Extra long press | Hold 3 seconds | Toggle power on / off |

Debounce threshold: 40ms.

---

## LED State Reference

| Color | Pattern | State |
|---|---|---|
| Green | Solid | Active — playing normally |
| Green | Blinking (1Hz) | Default volume mode active |
| Red/Blue | Alternating fast | Calibration in progress |
| Off | — | System powered off |

---

## Note Mapping

Eight notes mapped linearly across the 5–60cm playing range:

| Index | Note | Frequency | Approx. Distance |
|---|---|---|---|
| 0 | C4 | 261.63 Hz | ~5 cm |
| 1 | D4 | 293.66 Hz | ~13 cm |
| 2 | E4 | 329.63 Hz | ~22 cm |
| 3 | G4 | 392.00 Hz | ~30 cm |
| 4 | A4 | 440.00 Hz | ~38 cm |
| 5 | C5 | 523.25 Hz | ~47 cm |
| 6 | D5 | 587.33 Hz | ~55 cm |
| 7 | E5 | 659.25 Hz | ~60 cm |

---

## Serial Output Format

Baud rate: **115200**

```
RAW_DIST,SMOOTH_DIST,VEL,ACC,RAW_PHOTO,SMOOTH_PHOTO,NOTE,MODE
```

| Column | Type | Unit | Description |
|---|---|---|---|
| RAW_DIST | float | cm | Raw HC-SR04 reading (-1.0 = no hand detected) |
| SMOOTH_DIST | float | cm | EMA-smoothed distance (-1.0 = no hand) |
| VEL | float | cm/s | Hand velocity — first derivative of smoothed distance |
| ACC | float | cm/s² | Hand acceleration — second derivative |
| RAW_PHOTO | int | ADC counts | Raw 12-bit photoresistor reading (0–4095) |
| SMOOTH_PHOTO | float | 0.0–1.0 | Calibrated + EMA-smoothed normalized light level |
| NOTE | float | Hz | Current note frequency |
| MODE | int | 0 or 1 | 0 = photoresistor volume, 1 = default volume |

Example:
```
23.4,22.1,1.25,0.30,2927,0.89,440.0,0
```

Non-playing (no hand detected):
```
-1.0,-1.0,0.00,0.00,1843,0.42,261.6,0
```

System messages (not CSV — filtered by visualizer):
```
SYSTEM ON
CALIBRATION START
CALIBRATION DONE
SYSTEM OFF
```