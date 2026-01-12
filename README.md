# 🚛 Truck Weigh Station

An Arduino-based weight station prototype that monitors vehicle weight and controls access through automated barriers. Built for the CASA0016 coursework at UCL.

> **Stop right there, overloaded truck!** This system ensures heavy goods vehicles comply with weight regulations, keeping roads safe for everyone.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Arduino%20UNO-teal.svg)
![Status](https://img.shields.io/badge/status-Complete-brightgreen.svg)

---

## Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Hardware Components](#-hardware-components)
- [Circuit Diagram](#-circuit-diagram)
- [Software Architecture](#-software-architecture)
- [Installation](#-installation)
- [Calibration Guide](#-calibration-guide)
- [Usage](#-usage)
- [Project Gallery](#-project-gallery)
- [Future Improvements](#-future-improvements)
- [Acknowledgements](#-acknowledgements)

---

## Overview

Truck overloading is a serious safety concern—over 80% of heavy truck accidents are attributed to overloading violations. This prototype demonstrates an automated weight verification and enforcement system that:

- **Weighs** vehicles using a precision load cell
- **Displays** real-time weight readings on an LCD
- **Alerts** operators with an LED indicator when limits are exceeded
- **Controls** a barrier gate to physically enforce weight restrictions

The system uses a **300g threshold** to simulate real-world truck weight limits (scaled down for prototype demonstration).

<div align="center">
  <img src="/Images/Prototype2.JPG" width="50%" alt="The truck weigh station prototype" />
</div>

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| **Precision Weighing** | 1kg load cell with HX711 amplifier, accuracy within ±0.2g |
| **Live Display** | 16x2 LCD shows weight and access status in real-time |
| **Visual Warning** | Red LED instantly illuminates when weight exceeds threshold |
| **Barrier Control** | Servo-powered gate with 3-second delay for smooth operation |
| **EEPROM Storage** | Calibration values persist across power cycles |
| **Runtime Calibration** | Recalibrate via Serial Monitor without reflashing |

---

## Hardware Components

| Component | Specification | Purpose |
|-----------|---------------|---------|
| Arduino UNO | ATmega328P | Main controller |
| Load Cell | 1kg capacity | Weight sensing |
| HX711 | 24-bit ADC | Signal amplification |
| LCD Display | 16x2 characters | Weight & status display |
| Servo Motor | SG90 (180°) | Barrier gate control |
| LED | Red, 5mm | Overweight indicator |
| Battery Pack | 4.5V (3×AA) | External servo power |

---

## Circuit Diagram

<div align="center">
  <img src="/Images/TruckWeight_Circuit.png" width="70%" alt="Circuit connection" />
</div>

> ⚠️ **Important:** The servo motor draws up to 600mA, exceeding the Arduino's 500mA limit. Always use an external battery pack with a shared ground connection.

---

## Software Architecture

The system uses a **non-blocking state machine** architecture for smooth, responsive operation:

```
┌─────────────────┐
│   Load Cell     │
│   (HX711_ADC)   │
└────────┬────────┘
         │ Weight Data
         ▼
┌─────────────────┐
│  Main Loop      │──────► LCD Display
│  (State Logic)  │──────► LED Control
│                 │──────► Servo Control
└─────────────────┘
         │
         ▼
┌─────────────────┐
│ Serial Commands │
│  't' = Tare     │
│  'r' = Calibrate│
└─────────────────┘
```

**Key Code Pattern - Time-Delayed Servo Control:**

```cpp
// Boolean flags track system state
bool isAboveThreshold = false;
bool isServoAt0 = false;
unsigned long thresholdStartTime = 0;

// Non-blocking delay using millis()
if (weight > WEIGHT_THRESHOLD) {
    if (!isAboveThreshold) {
        isAboveThreshold = true;
        thresholdStartTime = millis();
    } else {
        // Close barrier after 3 seconds above threshold
        if (!isServoAt0 && (millis() - thresholdStartTime >= DELAY_TIME)) {
            myServo.write(0);
            isServoAt0 = true;
        }
    }
}
```

---

## Installation

### Prerequisites

- Arduino IDE (1.8.x or 2.x)
- [HX711_ADC Library](https://github.com/olkal/HX711_ADC) by Olav Kalhovd

### Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/Annie-Zhu1210/Truck-Weigh-Station.git
   cd Truck-Weigh-Station
   ```

2. **Install the HX711_ADC library**
   - Open Arduino IDE → Sketch → Include Library → Manage Libraries
   - Search for "HX711_ADC" by Olav Kalhovd
   - Click Install

3. **Upload the code**
   - Open `Weight_Screen_LED_Motor/Weight_Screen_LED_Motor.ino`
   - Select your board: Tools → Board → Arduino UNO
   - Select your port: Tools → Port → (your Arduino port)
   - Click Upload

4. **Calibrate the load cell** (see [Calibration Guide](#-calibration-guide))

---

## Calibration Guide

Proper calibration is essential for accurate weight readings. You'll need an object of **known precise weight** (e.g., a phone, calibration weight, or packaged food item).

### Initial Calibration

1. **Open Serial Monitor**
   - Tools → Serial Monitor
   - Set baud rate to `57600`

2. **Start calibration**
   - Send `r` to begin the calibration process

3. **Tare the scale**
   - Remove all weight from the load cell
   - Send `t` to set the zero point
   - Wait for "Tare complete" message

4. **Apply known weight**
   - Place your known weight on the load cell
   - Enter the exact weight in grams (e.g., `186.5`)
   - Press Enter

5. **Save to EEPROM**
   - When prompted, send `y` to save the calibration value
   - Note the displayed calibration value (e.g., `-1319.04`)

6. **Update the code**
   ```cpp
   // Replace this with your calibration value
   float calibrationValue = -1319.04;
   ```

### Quick Tare (Zero Reset)

If readings drift over time, you can re-tare without full recalibration:

1. Remove all weight from the load cell
2. Send `t` via Serial Monitor
3. Wait for "Tare complete!" on the LCD

### When to Recalibrate

- After moving or adjusting the load cell mounting
- If readings become consistently inaccurate
- After changing the enclosure configuration

---

## Usage

1. **Power up the system**
   - Connect Arduino via USB or external power
   - Turn on the battery pack for the servo

2. **Wait for initialisation**
   - LCD displays "Initialising..." then "Ready!"

3. **Test with weights**
   - Place objects on the weighing platform
   - Observe LCD readings and system responses:

   | Weight | LCD Message | LED | Barrier |
   |--------|-------------|-----|---------|
   | ≤ 300g | "Please go through" | OFF | Open (90°) |
   | > 300g | "Please stop" | ON | Closed (0°) after 3s |

4. **Adjust threshold (optional)**
   ```cpp
   const float WEIGHT_THRESHOLD = 300.0;  // Change this value
   const unsigned long DELAY_TIME = 3000;  // Delay in milliseconds
   ```

---

## Project Gallery

<!-- TODO: Add your project images here -->

### System Overview
<!-- ![System Overview](images/system_overview.jpg) -->
*Add your system overview image here*

### Enclosure Interior
<!-- ![Enclosure Interior](images/enclosure_interior.jpg) -->
*Add your enclosure interior image here*

### Barrier in Action
<!-- ![Barrier Closed](images/barrier_closed.jpg) -->
*Add your barrier demonstration image here*

### Testing
<!-- ![Testing](images/testing.jpg) -->
*Add your testing image here*

---

## Future Improvements

- [ ] **Adjustable threshold interface** - Select vehicle types with preset weight limits
- [ ] **Smooth servo motion** - Implement incremental angle transitions
- [ ] **Multi-axle measurement** - Add multiple load cells for axle load distribution
- [ ] **Data logging** - Record vehicle weights with timestamps
- [ ] **Wireless connectivity** - Send alerts to remote monitoring systems

---

## Acknowledgements

- **[HX711_ADC Library](https://github.com/olkal/HX711_ADC)** by Olav Kalhovd - For excellent calibration and EEPROM functionality
- **[Servo Barrier Design](https://www.thingiverse.com/thing:2451583)** by V. Hovan - For the 3D printed barrier arm
- **Simon Gosling** (UCL CE Lab) - For enclosure design recommendations
- **Claude AI** (Anthropic) - For code improvements and documentation assistance

---

## 📄 License

This project is open source and available under the [MIT License](LICENSE).

---

<p align="center">
  Made with ❤️ for CASA0016 @ UCL
</p>
