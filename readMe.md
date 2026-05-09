#  BONBIBI  (Intelligent Forest Protection & Illegal Logging Detection System)
## A project by Team NO Surprises for the 1st GRIC(Global Robotics and Innovation Consortium) project league- Regional Round

BONBIBI is a low-cost distributed forest surveillance network designed to detect illegal tree-cutting activity inside sensitive forest ecosystems such as the Sundarbans.

---

# Objectives
- Detect suspicious tree-cutting activity
- Reduce false positives using sensor fusion
- Send real-time alerts from remote locations
- Provide tactical monitoring dashboard
- Enable scalable forest surveillance networks

---

# How it works

A tree experiences:
- vibration spikes
- repeated impact patterns
- distinct acoustic signatures

during illegal cutting.

BONBIBI combines:
- accelerometer data
- vibration sensor triggers
- microphone RMS energy (FFT to be included later)

to estimate confidence level of logging activity.

---

# System Architecture

```txt
        ┌──────────────────┐
        │  SW-420 Sensor   │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │    ESP32 MCU     │
        │                  │
        │ Sensor Fusion    │
        │ Event Analysis   │
        └───────┬──────────┘
                │
        ┌───────┴──────────┐
        │                  │
        ▼                  ▼
┌──────────────┐   ┌────────────────┐
│ MPU6050 IMU  │   │ INMP441 Mic    │
└──────────────┘   └────────────────┘
                │
                ▼
        ┌──────────────────┐
        │ Confidence Logic │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │ LoRa Transmitter │
        └────────┬─────────┘
                 │
                 ▼
        ┌──────────────────┐
        │ Mission Control  │
        │ Dashboard        │
        └──────────────────┘
```

---

# 🔩 Hardware Components

| Component | Purpose |
|---|---|
| ESP32 | Main microcontroller |
| MPU6050 | Detects acceleration and impacts |
| INMP441 | Captures acoustic energy |
| SW-420 | Wake-on-vibration trigger |
| LoRa Module | Long-range communication |
| GPS Module (Optional) | Forest node location |
| Battery Pack | Remote deployment power |

---

# Detection logic





---


---
# Cmmunication

The system uses LoRa for:
- long-range operation
- low power usage
- remote forest deployment

 architecture:
- node-to-gateway

---


