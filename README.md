# people-counter-esp32

## Overview

Smart People Counter System is an IoT-based project developed using ESP32, ultrasonic sensors, Firebase Realtime Database and a web dashboard.

The system detects people entering and leaving a room, tracks occupancy in real time, displays information on an OLED screen and synchronizes data with Firebase for remote monitoring through a web application.

---

# Features

- Real-time people counting
- Entry/exit direction detection
- OLED live display
- Green/Red occupancy status LEDs
- Firebase Realtime Database integration
- Live web dashboard
- Occupancy history logging
- Peak occupancy tracking
- Average occupancy calculation
- Remote counter reset
- Remote ESP32 restart
- Dynamic max capacity update from web app

---

# Hardware Components

- ESP32
- 2x HC-SR04 Ultrasonic Sensors
- OLED SSD1306 Display
- Red LED
- Green LED
- Breadboard
- Resistors
- Jumper wires

---

# Technologies Used

## Embedded / IoT
- Arduino IDE
- ESP32
- C++

## Cloud / Backend
- Firebase Realtime Database

## Frontend
- HTML
- CSS
- JavaScript
- Chart.js

---

# System Architecture

ESP32 reads data from two ultrasonic sensors to determine movement direction:

- Sensor 1 → Sensor 2 = Person entered
- Sensor 2 → Sensor 1 = Person exited

The system:
1. Updates the OLED display
2. Controls occupancy LEDs
3. Sends data to Firebase
4. Updates the web dashboard in real time

---

# Firebase Data Structure

```json
peopleCounter
|
|-- average
|-- current
|-- history
|-- max
|-- peak
|-- resetCounter
|-- restartESP
```

---

# Web Dashboard

The dashboard provides:
- Current occupancy
- Maximum capacity
- Average occupancy
- Peak occupancy
- Occupancy graph
- Reset button
- ESP32 restart button
- Live Firebase synchronization

---


# How To Run

## ESP32
1. Open `.ino` file in Arduino IDE
2. Install required libraries
3. Configure:
   - WiFi credentials
   - Firebase credentials
4. Upload code to ESP32

## Web App
1. Open project in VS Code
2. Run local server
3. Open dashboard in browser


---

# Project Goals

This project was created to demonstrate:
- IoT architecture
- Hardware-software integration
- Real-time cloud synchronization
- Embedded systems programming
- Smart room monitoring systems
