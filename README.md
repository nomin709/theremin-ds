# Theremin DS — IoT Signal Processing & Gesture Recognition

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
**Embedded:** Arduino C++ (ESP32 core v3.x)
**Visualization:** Processing 4
**Analysis:** Python, pandas, numpy, scipy
**ML:** scikit-learn, PyTorch
**Dashboard:** Plotly Dash

## Project Structure
...

## How to Run
...

## Findings
...
