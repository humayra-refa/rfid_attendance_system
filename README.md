# RFID-Based Attendance System

A contactless, automated attendance management system built with ESP32, Firebase Firestore, and a real-time web dashboard.

**Live Demo:** https://rfid-attendence-f28aa.web.app/

---

## Hardware Prototype


*RC522 RFID module + ESP32 on veroboard with red/green LEDs and buzzer — fully soldered and compact.*

---

## Overview

Users tap an RFID card on the RC522 reader. The ESP32 reads the card UID, verifies it, and sends attendance data to Firebase Firestore over WiFi. The web dashboard updates instantly. Green LED + buzzer confirm a valid scan; red LED signals an unauthorized card.

---

## Project Structure

```
rfid-final/
├── assets/              # Images for README
│   └── hardware.jpg
├── esp32_code/          # Arduino firmware (.ino)
├── public/              # Web dashboard
│   ├── dashboard.html
│   ├── login.html
│   ├── students.html
│   ├── attendance.html
│   ├── index.html
│   ├── firebase.js
│   └── style.css
├── firebase.json
├── firestore.rules
├── firestore.indexes.json
└── SETUP_GUIDE.md
```

---

## Hardware

| Component | Qty |
|---|---|
| ESP32 Development Board | 1 |
| RFID Module (MFRC522) | 1 |
| RFID Card / Tag | 1+ |
| Green LED | 1 |
| Red LED | 1 |
| 320Ω Resistor | 2 |
| Buzzer Module | 1 |
| Veroboard | 1 |
| Jumper Wires | Several |

### Wiring

**RC522 to ESP32**

| RC522 | ESP32 |
|---|---|
| SDA | GPIO 5 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| RST | GPIO 22 |
| GND | GND |
| 3.3V | 3.3V |

**Outputs**

| Component | GPIO |
|---|---|
| Green LED | GPIO 12 |
| Red LED | GPIO 14 |
| Buzzer | GPIO 13 |

---

## Tech Stack

| Layer | Technology |
|---|---|
| Firmware | Arduino IDE, Embedded C++ |
| Microcontroller | ESP32 (WiFi-enabled) |
| Database | Firebase Firestore (NoSQL, real-time) |
| Auth | Firebase Authentication |
| Hosting | Firebase Hosting |
| Frontend | HTML5, CSS3, Vanilla JS |
| Time Sync | NTPClient (UTC+6) |
| Backup | Google Apps Script + Sheets |

---

## Setup

### Firmware

1. Install [Arduino IDE](https://www.arduino.cc/en/software) and add ESP32 board support.

2. Install required libraries: `MFRC522`, `FirebaseESP32`, `NTPClient`

3. Open `esp32_code/` and update credentials:

   ```cpp
   #define WIFI_SSID        "your_wifi_name"
   #define WIFI_PASSWORD    "your_wifi_password"
   #define FIREBASE_HOST    "your-project.firebaseio.com"
   #define FIREBASE_AUTH    "your_database_secret"
   ```

4. Select board: **ESP32 Dev Module** and upload.

### Web Dashboard

```bash
npm install -g firebase-tools
firebase login
firebase deploy
```

---

## How It Works

```
Card scanned
    → RC522 reads UID (via SPI)
    → ESP32 checks authorization
    → Valid:   Green LED + Buzzer + attendance written to Firestore
    → Invalid: Red LED only
    → Dashboard updates in real time via onSnapshot()
```

---

## Features

- Contactless RFID scanning
- Real-time Firestore sync
- Admin login with Firebase Auth
- Live dashboard with attendance stats
- CSV export
- Google Sheets auto-backup
- Mobile-responsive UI
- Duplicate entry prevention

---

## Team — Zenith

| Name | Role |
|---|---|
| Maisha Osman Umama | System Integrator |
| Iffat Humayra Refa | Embedded Coding & Testing |
| Ashfaque Ur Rahman | Hardware & Electrical Design |
| Mahfuz Anam Arnob | Hardware Implementation |

---

*Developed for academic/lab use. Free to use with attribution.*
