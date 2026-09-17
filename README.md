# ESP32 Theremin - Embedded Signal Processing & Real-Time Visualization

> A contactless two-hand musical instrument built on ESP32 — controlled entirely by hand position in the air, with real-time signal processing and live visualization.

---

## Overview

This project turns an ESP32 microcontroller into a **theremin** — an electronic instrument played without physical contact. Two sensors track each hand independently:

- **Left hand** → HC-SR04 ultrasonic sensor → controls **pitch**
- **Right hand** → Photoresistor → controls **volume**
- **Button** → multi-gesture control (calibrate, default volume, power)

The ESP32 runs a **non-blocking 30Hz sampling loop** with on-device exponential smoothing and real-time kinematics (velocity and acceleration). All sensor data is streamed over serial as a structured CSV, parsed and visualized live in a **Processing 4 dashboard**.

---

<!-- ## Demo

> 🎥 *Demo video coming soon* -->

---

## Features

### Instrument
- Contactless pitch control via ultrasonic distance (5–60cm range)
- Contactless volume control via ambient light level
- 8-note scale mapped across the playing range (C4 → E5)
- Clean audio output through PAM8403 Class D amplifier + speaker

### Firmware
- Deterministic **30Hz non-blocking** sampling loop
- **Exponential smoothing** on both sensor channels
- **On-device kinematics** — velocity (cm/s) and acceleration (cm/s²) computed in real time
- **5-second auto-calibration** for photoresistor — adapts to any lighting condition
- **Multi-gesture button** — single press, double press, and 3-second long press
- Dynamic RGB LED state indicators

### Visualization
- Real-time pitch and volume bar displays
- Scrolling 10-second time-series graph (combined or split view)
- Live sensor readouts — distance, raw light, normalized light
- Calibration and system state overlays

---

## Hardware

| Component | Purpose |
|---|---|
| ESP32 Dev Module (ESP32-D0WD-V3) | Microcontroller |
| HC-SR04 Ultrasonic Sensor | Left hand — pitch control |
| Photoresistor (LDR) | Right hand — volume control |
| PAM8403 Amplifier Module | Audio amplification |
| Small Speaker (8Ω) | Audio output |
| RGB LED (Common Cathode) | System state indicator |
| Pushbutton | Multi-gesture control |
| 220Ω resistors × 3 | RGB LED current limiting |
| 1kΩ + 2kΩ resistors | HC-SR04 ECHO voltage divider |
| 10kΩ resistor | Photoresistor pull-down |

> See [`esp32/README.md`](esp32/README.md) for full wiring instructions.

---

## Repository Structure

```
theremin-ds/
│
├── README.md
│
├── esp32/
│   ├── theremin.ino
│   └── README.md
│
└── processing/
    ├── visualizer.pde
    └── README.md
```

---

## Quick Start

### 1. Upload Firmware
See [`esp32/README.md`](esp32/README.md) for full setup.

```
Board:        ESP32 Dev Module
Upload speed: 115200
Baud rate:    115200
```

### 2. Run Visualization
See [`processing/README.md`](processing/README.md) for full setup.

```
1. Close Arduino Serial Monitor
2. Open processing/visualizer.pde in Processing 4
3. Run once → find your ESP32 port in the console
4. Update Serial.list()[X] to match your port
5. Run again
```

### 3. Button gestures

| Gesture | How | Action |
|---|---|---|
| Single press | Short press | Run 5-second photoresistor calibration |
| Double press | Two presses within 350ms | Toggle default volume mode (fixed 40%) |
| Long press | Hold 3 seconds | Power on / off |

**Calibrating:**
Press the button once. The RGB LED alternates red/blue for 5 seconds. Wave your right hand over the photoresistor between fully covered and fully uncovered to set the volume range. LED returns to solid green when done.

**Default volume mode:**
Double press to toggle. Fixes volume at 40% regardless of light level — useful if you want to focus purely on pitch. LED blinks green slowly while active.

**Power:**
Hold the button for 3 seconds to toggle power off. Audio stops and LED turns off. Hold again to power back on.

---

## Tech Stack

| Layer | Technology |
|---|---|
| Hardware | ESP32-D0WD-V3, HC-SR04, Photoresistor, PAM8403 |
| Firmware | Arduino C++, ESP32 core v3.x, LEDC PWM |
| Visualization | Processing 4 |

---

## License

MIT

<!-- Future updated theremin prject readme.md -->
<!-- # Theremin DS — IoT Signal Processing & Gesture Recognition

A full data science pipeline built on an ESP32-based theremin instrument.
Raw sensor data flows from embedded hardware through signal processing,
feature engineering, and machine learning gesture classification,
visualized in a real-time Plotly Dash dashboard.

## Repository Structure
```
theremin-ds/
│
├── README.md                    
│
├── esp32/
│   ├── theremin.ino             ← main firmware
│   └── README.md                ← wiring diagram, pin table
│
├── processing/
│   ├── visualizer/
│   │   └── visualizer.pde
│   └── README.md                ← how to run, port setup
│
├── data/
│   ├── raw/                     ← raw CSV logs from sessions
│   ├── processed/               ← cleaned, feature-engineered
│   └── labeled/                 ← gesture-labeled dataset
│
├── notebooks/
│   ├── 01_eda.ipynb
│   ├── 02_signal_processing.ipynb
│   ├── 03_feature_engineering.ipynb
│   ├── 04_gesture_classification.ipynb
│   └── 05_deep_learning.ipynb
│
├── dashboard/
│   ├── app.py                   ← Plotly Dash app
│   ├── components/              ← UI panels
│   └── requirements.txt
│
├── models/
│   ├── gesture_classifier.joblib
│   └── model_card.md            ← what it does, accuracy, limitations
│
├── assets/
│   ├── demo.gif                 ← recording of it working
│   ├── wiring_diagram.png
│   └── plots/                   ← key plots from notebooks
│
├── requirements.txt             ← Python dependencies
└── .gitignore
```

## Demo
[insert demo.gif here]

## Pipeline Overview
[insert architecture diagram]

## Key Results
| Model          | F1 Score | Latency |
|----------------|----------|---------|
| Rule-based     | 71%      | <1ms    |
| Random Forest  | 89%      | 12ms    |
| 1D CNN         | 94%      | 31ms    |

## Tech Stack
**Hardware:** ESP32, HC-SR04, Photoresistor, PAM8403 amp \
**Embedded:** Arduino C++ (ESP32 core v3.x) \
**Visualization:** Processing 4 \
**Analysis:** Python, pandas, numpy, scipy \
**ML:** scikit-learn, PyTorch \
**Dashboard:** Plotly Dash

## Project Structure
...

## How to Run
...

## Findings
... -->