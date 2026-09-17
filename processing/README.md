# Processing Visualizer — `visualizer.pde`

A real-time dashboard built in Processing 4 that parses the ESP32 serial stream and renders live instrument state — pitch, volume, sensor readings, and scrolling time-series graphs.

---

## Requirements

- [Processing 4](https://processing.org/download) — free download
- ESP32 running `theremin.ino` connected via USB

---

## Setup

### 1. Find your serial port

Run the sketch once. The console at the bottom of Processing will print all available serial ports, for example:

```
[0]  "/dev/cu.Bluetooth-Incoming-Port"
[1]  "/dev/cu.usbserial-10"
[2]  "/dev/cu.usbmodem14101"
```

Find the port that matches your ESP32 — it will contain `usbserial` or `usbmodem` on Mac/Linux, or `COM3` / `COM4` on Windows.

### 2. Update the port index

Open `visualizer.pde` and find line 44:

```java
myPort = new Serial(this, Serial.list()[0], 115200);
```

Replace `0` with the index number matching your ESP32 port:

```java
myPort = new Serial(this, Serial.list()[1], 115200); // example
```

### 3. Close Arduino Serial Monitor

Both Arduino Serial Monitor and Processing try to open the same serial port. If Serial Monitor is open, Processing will crash on startup with a port-busy error.

Always close Serial Monitor before running the visualizer.

### 4. Run

Click the **Run** button (▶) in Processing. The dashboard should open immediately and begin displaying data.

---

## Interface

### Pitch Bar
A vertical bar on the left showing the current note frequency in Hz. Fills upward as pitch increases. Range: 250–700 Hz (C4 to E5).

Color: **yellow/gold**

### Volume Bar
A vertical bar showing the normalized photoresistor level — how open or covered your right hand is. Displays `DEFAULT` label when default volume mode is active.

Color: **light blue**

### Scrolling Graph
A 300-sample time-series history (10 seconds at 30Hz) showing pitch and volume over time.

Two view modes:
- **Combined** — both signals overlaid on one graph
- **Separate** — pitch and volume on independent panels

Toggle between views with the `M` key.

### Info Panel
Live readouts in the bottom-left corner:

| Field | Description |
|---|---|
| Power | System on / off state |
| Calibrated | Whether photoresistor has been calibrated |
| Playing | Whether a hand is detected in range |
| Distance | Smoothed HC-SR04 reading in cm (or "No Hand") |
| Raw Light | Raw 12-bit ADC reading from photoresistor |
| Norm Light | Normalized light level (0.00–1.00) |

### System Overlays

**Calibration banner** — displayed at the bottom of the screen while the ESP32 is in calibration mode (LED alternating red/blue). The screen also dims slightly.

**System message** — brief notification displayed center-screen when the ESP32 sends a system event (power on, power off, calibration start/done).

---

## Keyboard Controls

| Key | Action |
|---|---|
| `P` | Pause / resume graph updates |
| `R` | Reset graph history to zero |
| `M` | Toggle combined / separate graph view |

---

## Serial Protocol

The visualizer expects exactly **8 comma-separated values** per line at **115200 baud**:

```
RAW_DIST,SMOOTH_DIST,VEL,ACC,RAW_PHOTO,SMOOTH_PHOTO,NOTE,MODE
```

Lines that don't match this format (system messages, malformed reads) are silently ignored.

Lines starting with `SYSTEM` or `CALIBRATION` are recognized as status messages and used to update the system overlay — not plotted.

---

## Troubleshooting

**Blank screen / no data**
- Make sure Arduino Serial Monitor is closed
- Check that the port index in `Serial.list()[X]` matches your ESP32
- Verify the ESP32 is running and outputting data (check Serial Monitor first, then close it)

**Graph is frozen**
- Press `P` to unpause

**Port index keeps changing**
- On Mac/Linux, the index can shift when devices are plugged/unplugged. Re-run once to reprint the port list and update the index.

**Processing crashes on start**
- Port is already in use — close Serial Monitor or any other app reading the same port