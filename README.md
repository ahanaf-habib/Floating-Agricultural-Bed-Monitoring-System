# 🌱 Floating Garden Management System

An IoT-based smart monitoring and management system designed for floating agriculture, traditionally known as **Dhap (ধাপ)** in Bangladesh. The system uses an ESP32 and multiple sensors to continuously monitor the environmental, agricultural, structural, and safety conditions of floating agricultural beds.

The system processes sensor data in real time and provides automated alerts through Telegram whenever monitored conditions exceed predefined safety or agricultural thresholds.

---

## 📌 Project Overview

Floating agriculture is a traditional cultivation method used in flood-prone and waterlogged regions of Bangladesh, including Gopalganj, Barishal, Pirojpur, and Madaripur.

Floating beds are constructed using water hyacinth and other aquatic plants, creating a biodegradable and nutrient-rich growing medium that can remain afloat as water levels change.

However, decomposition, uneven weight distribution, waves, anchor failure, temperature, moisture, salinity, pH, and turbidity can affect the stability and productivity of these beds.

This project aims to provide an economical IoT-based solution for continuously monitoring these conditions and notifying farmers when corrective action may be required.

---

## 🎯 Objectives

- Monitor the environmental and agricultural conditions of floating beds.
- Detect structural instability and excessive bed immersion.
- Monitor water and soil-related parameters.
- Detect possible animal intrusion.
- Detect GPS drift caused by possible anchor failure.
- Provide real-time alerts to farmers through Telegram.
- Reduce crop loss through early detection of abnormal conditions.
- Provide an affordable monitoring solution suitable for rural Bangladesh.

---

## ⚙️ Main Features

### 🌱 Floating-Bed Health & Environmental Monitoring

The system continuously monitors:

- Soil temperature
- Top-layer soil moisture
- Water TDS/salinity
- Water pH
- Water turbidity

When monitored parameters move outside their predefined ranges, the system generates an appropriate alert.

### ⚖️ Structural & Floating Safety

The system monitors the physical condition and stability of the floating bed using:

- Water level / immersion depth
- Pitch and roll
- Sudden acceleration
- Unusual movement
- Geo fenching using GPS location

The MPU-6050 is used to detect excessive tilt and sudden movement, while the GPS module is used to monitor the bed's location.

A virtual geofence of approximately **200 meters** around the original anchor point is used to detect possible drifting.

### 🐀 Intrusion Detection

A PIR motion sensor is used to detect possible intrusion from animals such as:

- Rats
- Ducks
- Birds
- Other warm-blooded animals

A vibration sensor may also be used to improve intrusion detection reliability.

When an intrusion is detected, the system can send an alert through Telegram and activate a local buzzer or ultrasonic repeller.

### 📱 Telegram Monitoring & Alerts

Telegram is used as the remote notification and monitoring interface.

The system can:

- Send automatic alerts when thresholds are exceeded.
- Provide current sensor readings.
- Notify the farmer about structural or environmental problems.
- Provide status information through commands such as `/status`.

This eliminates the need for a dedicated mobile application.

---

## 🔧 Hardware Components

| Component | Purpose |
|---|---|
| **ESP32** | Main controller, data processing and wireless communication |
| **MPU-6050** | Pitch, roll, acceleration and movement monitoring |
| **NEO-6M GPS** | Location tracking and geofence monitoring |
| **Water Level Sensor** | Floating-bed immersion/depth monitoring |
| **DS18B20 Waterproof Temperature Sensor** | Subsurface/root-zone temperature monitoring |
| **Capacitive Soil Moisture Sensor** | Top-layer soil moisture monitoring |
| **TDS Sensor** | Dissolved salts and salinity monitoring |
| **Analog pH Sensor** | Water pH monitoring |
| **Turbidity Sensor** | Water clarity monitoring |
| **PIR Motion Sensor** | Animal/intrusion detection |
| **Vibration Sensor** | Additional movement/intrusion detection |
| **Buzzer / Ultrasonic Repeller** | Local animal deterrent |
| **Telegram Bot** | Remote monitoring and alert system |

---

## 🧠 System Architecture

The ESP32 acts as the central controller of the system.

'
                 ┌─────────────────────┐
                 │       ESP32         │
                 │   Main Controller   │
                 └──────────┬──────────┘
                            │
       ┌────────────────────┼────────────────────┐
       │                    │                    │
       ▼                    ▼                    ▼
  Environmental       Structural/Safety     Intrusion
     Sensors              Sensors            Sensors
       │                    │                    │
       ├─ Temperature       ├─ MPU-6050         ├─ PIR
       ├─ Soil Moisture     ├─ GPS              └─ Vibration
       ├─ TDS               └─ Water Level
       ├─ pH
       └─ Turbidity
                            │
                            ▼
                  ┌───────────────────┐
                  │   Data Processing  │
                  │ & Threshold Check  │
                  └─────────┬─────────┘
                            │
                  ┌─────────┴─────────┐
                  │                   │
              Normal              Abnormal
                  │                   │
                  ▼                   ▼
            Continue             Alert System
            Monitoring                │
                                      ▼
                              ┌───────────────┐
                              │    Telegram   │
                              │     Alerts    │
                              └───────────────┘
