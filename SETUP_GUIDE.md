# 📡 RFID Smart Attendance System — Complete Setup Guide

## System Overview
A complete IoT attendance system: students tap RFID cards → ESP32 detects the UID → sends data to Firebase → appears live on the web dashboard.

---

## FOLDER STRUCTURE
```
rfid-attendance/
├── index.html           → Redirects to login
├── login.html           → Admin login page
├── dashboard.html       → Main live dashboard
├── attendance.html      → Full records + search + export
├── students.html        → Student registration
├── style.css            → All styling
├── firebase.js          → Firebase SDK + helpers
└── esp32_code/
    ├── rfid_attendance.ino     → ESP32 Arduino code
    └── google_apps_script.js  → Google Sheets backend
```

---

## STEP 1 — FIREBASE SETUP

### 1.1 Create Firebase Project
1. Go to https://console.firebase.google.com
2. Click **Add project** → Name it `rfid-attendance`
3. Disable Google Analytics (optional) → **Create project**

### 1.2 Enable Firestore Database
1. Left sidebar → **Build → Firestore Database**
2. Click **Create database**
3. Choose **Start in test mode** (for development)
4. Select a region close to you → **Done**

### 1.3 Enable Authentication
1. Left sidebar → **Build → Authentication**
2. Click **Get started**
3. Go to **Sign-in method** tab
4. Enable **Email/Password** → Save
5. Go to **Users** tab → **Add user**
   - Email: `admin@school.edu`
   - Password: (choose a strong password)

### 1.4 Get Firebase Config
1. Click the gear icon → **Project settings**
2. Scroll to **Your apps** section
3. Click **</>** (Web app) → Register with name `rfid-web`
4. Copy the `firebaseConfig` object

### 1.5 Update firebase.js
Replace the placeholder in `firebase.js`:
```javascript
const firebaseConfig = {
  apiKey: "AIzaSy...",
  authDomain: "rfid-attendance-xxxxx.firebaseapp.com",
  projectId: "rfid-attendance-xxxxx",
  storageBucket: "rfid-attendance-xxxxx.appspot.com",
  messagingSenderId: "123456789",
  appId: "1:123456789:web:abcdef"
};
```

### 1.6 Firestore Rules (production-ready)
Go to **Firestore → Rules** and paste:
```
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    // Public read for attendance (ESP32 writes)
    match /attendance/{doc} {
      allow read: if request.auth != null;
      allow write: if true;  // Allow ESP32 to write
    }
    // Students — admin only
    match /students/{uid} {
      allow read, write: if request.auth != null;
    }
    // Admin writes for production: allow write: if request.auth != null;
  }
}
```

---

## STEP 2 — DATABASE STRUCTURE

### Collection: `students`
Document ID = RFID UID (e.g., `A1B2C3D4`)
```json
{
  "name": "Rahul Ahmed",
  "studentId": "221-15-1234",
  "uid": "A1B2C3D4",
  "department": "Engineering",
  "email": "rahul@university.edu",
  "phone": "01712345678",
  "createdAt": <timestamp>
}
```

### Collection: `attendance`
Auto-generated document ID
```json
{
  "name": "Rahul Ahmed",
  "studentId": "221-15-1234",
  "uid": "A1B2C3D4",
  "department": "Engineering",
  "status": "Present",
  "time": "10:30 AM",
  "date": "2026-05-16",
  "timestamp": <serverTimestamp>
}
```

---

## STEP 3 — ESP32 SETUP

### 3.1 Install Arduino IDE
Download from: https://www.arduino.cc/en/software

### 3.2 Add ESP32 Board
1. File → Preferences → Additional Board URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Tools → Board → Board Manager → Search `esp32` → Install

### 3.3 Install Libraries
Tools → Manage Libraries → Install each:
- **MFRC522** by GithubCommunity
- **LiquidCrystal I2C** by Frank de Brabander
- **ArduinoJson** by Benoit Blanchon

### 3.4 Get Firebase Web API Key
Firebase Console → Project Settings → General → Web API Key
(looks like `AIzaSy...`)

### 3.5 Configure ESP32 Code
Open `rfid_attendance.ino` and update:
```cpp
const char* WIFI_SSID     = "YourWiFiName";
const char* WIFI_PASSWORD = "YourWiFiPassword";
const char* FIREBASE_API_KEY = "AIzaSy...";    // From step 3.4
const char* FIREBASE_PROJECT  = "rfid-attendance-xxxxx"; // Your project ID
```

### 3.6 NTP Time Setup
Add to `setup()` after WiFi connects:
```cpp
configTime(6 * 3600, 0, "pool.ntp.org");  // UTC+6 for Bangladesh
```

### 3.7 Upload & Test
1. Select board: **Tools → Board → ESP32 Dev Module**
2. Select port: **Tools → Port → COMxx**
3. Upload (→ button)
4. Open Serial Monitor (115200 baud)
5. Scan a card — the UID will print

---

## STEP 4 — GOOGLE SHEETS INTEGRATION

### 4.1 Open Your Google Sheet
URL: https://docs.google.com/spreadsheets/d/1P2ha8nDAZhJvj-msMAYSVoVtkTgzFKA_4Rkki2IkNl8/edit

### 4.2 Open Apps Script
Extensions → Apps Script → Replace all code with `google_apps_script.js`

### 4.3 Deploy as Web App
1. Click **Deploy** → **New deployment**
2. Click the gear icon → **Web app**
3. Description: `RFID Attendance v1`
4. Execute as: **Me**
5. Who has access: **Anyone**
6. Click **Deploy** → **Authorize** → **Allow**
7. **Copy the Web App URL** (looks like `https://script.google.com/macros/s/XXXX.../exec`)

### 4.4 Add URL to ESP32
```cpp
const char* GOOGLE_SHEET_URL = "https://script.google.com/macros/s/XXXX.../exec";
```

### 4.5 Test the Endpoint
Open in browser:
```
https://script.google.com/macros/s/XXXX.../exec?name=Test+Student&uid=A1B2C3D4&studentId=221-15-0001&department=Engineering&status=Present
```
Should return: `{"success":true,"message":"Attendance recorded for Test Student"}`

---

## STEP 5 — WEB HOSTING (Firebase Hosting)

### 5.1 Install Firebase CLI
```bash
npm install -g firebase-tools
firebase login
```

### 5.2 Initialize Hosting
```bash
cd rfid-attendance
firebase init hosting
# Select your project
# Public directory: . (current folder)
# Single page app: No
# Overwrite index.html: No
```

### 5.3 Deploy
```bash
firebase deploy --only hosting
```
Your site will be live at: `https://rfid-attendance-xxxxx.web.app`

### Alternative: Netlify (easier, free)
1. Go to https://netlify.com
2. Drag & drop the `rfid-attendance` folder
3. Done! Live in 30 seconds.

---

## STEP 6 — REGISTERING STUDENTS

### Method 1: Web Dashboard
1. Open dashboard → Students → **+ Register Student**
2. Fill form (Name, Student ID, Department)
3. For UID: scan the card on ESP32 and read Serial Monitor
4. Copy the UID (e.g., `A1B2C3D4`) into the form

### Method 2: Firestore Console
1. Firebase Console → Firestore → students collection
2. Add document with ID = card UID
3. Add all fields manually

---

## WIRING DIAGRAM

```
ESP32                    RC522
─────                    ─────
GPIO 5  ──────────────── SDA
GPIO 18 ──────────────── SCK
GPIO 23 ──────────────── MOSI
GPIO 19 ──────────────── MISO
GPIO 22 ──────────────── RST
3.3V    ──────────────── 3.3V
GND     ──────────────── GND

ESP32                    LCD I2C
─────                    ───────
GPIO 21 ──────────────── SDA
GPIO 4  ──────────────── SCL
3.3V    ──────────────── VCC
GND     ──────────────── GND

ESP32                    LEDs & Buzzer
─────                    ─────────────
GPIO 2  ── [220Ω] ────── Green LED (+)
GPIO 15 ── [220Ω] ────── Red LED (+)
GPIO 13 ──────────────── Buzzer (+)
GND     ──────────────── All (-)
```

---

## DEBUGGING TIPS

### ESP32 won't connect to WiFi
- Check SSID/password (case-sensitive)
- ESP32 only supports 2.4GHz WiFi
- Try moving closer to router

### RFID won't read cards
- Check SPI pin connections
- Ensure 3.3V power (NOT 5V)
- Verify RST pin not conflicting with LCD SCL

### Firebase POST returning 403
- Check Firestore rules allow writes
- Verify API key is correct
- Add `?key=YOUR_API_KEY` to the URL

### LCD shows garbage / nothing
- Try I2C address 0x3F instead of 0x27
- Run I2C scanner sketch to find address
- Check SDA/SCL pins

### Student not found on scan
- First register the student on the dashboard
- UID must match exactly (uppercase, no spaces)
- Verify student document ID = UID in Firestore

---

## VIVA / INTERVIEW Q&A

**Q: How does RFID work?**
A: RFID uses radio frequency electromagnetic fields to read data from tags. The RC522 reader emits RF at 13.56MHz, which powers passive tags and reads their unique ID (UID).

**Q: Why Firebase?**
A: Firebase provides real-time database, authentication, and hosting in one platform with no backend server needed — perfect for IoT prototypes.

**Q: How is duplicate attendance prevented?**
A: ESP32 tracks recently scanned UIDs in memory with timestamps. If same UID is scanned within 30 seconds, it's ignored.

**Q: What is the data flow?**
A: Card → RC522 (SPI) → ESP32 (reads UID, looks up student) → Firebase Firestore (POST via REST API) → Web Dashboard (Firestore listener → real-time update)

**Q: How does real-time update work on the web?**
A: Firebase Firestore's `onSnapshot()` listener maintains a WebSocket connection. When ESP32 writes new data, Firestore pushes it instantly to all connected browsers.

**Q: What happens if WiFi is lost?**
A: Currently, the record is lost. Future improvement: store to SD card or EEPROM, then sync when WiFi restores.

---

## FUTURE IMPROVEMENTS

1. **Offline queue** — Store failed records in ESP32 EEPROM, sync later
2. **Telegram/email alerts** — Notify parents when student marked absent
3. **Fingerprint + RFID** — Two-factor attendance verification
4. **Mobile app** — React Native app for teachers
5. **Analytics** — Monthly charts, department-wise trends
6. **NFC cards** — Upgrade to NFC for smartphones
7. **QR fallback** — If RFID fails, use QR code scan
8. **SD card logging** — Local backup even without internet
9. **OTA updates** — Update ESP32 firmware over WiFi
10. **Multi-room** — Multiple ESP32 readers for different classrooms

---

## SYSTEM WORKFLOW DIAGRAM

```
┌─────────────┐     SPI      ┌──────────┐    WiFi    ┌─────────────────┐
│  RFID Card  │ ──────────▶  │  RC522   │ ─────────▶ │     ESP32       │
│  (Student)  │              │  Reader  │            │                 │
└─────────────┘              └──────────┘            │ 1. Read UID     │
                                                     │ 2. Lookup DB    │
                                                     │ 3. POST attend. │
                                                     └────────┬────────┘
                                                              │ HTTPS REST
                              ┌───────────────────────────────┴──────────────────┐
                              │                                                   │
                    ┌─────────▼──────────┐                           ┌────────────▼─────┐
                    │  Firebase Firestore │                           │  Google Sheets    │
                    │  (attendance coll.) │                           │  (Apps Script)    │
                    └─────────┬──────────┘                           └──────────────────┘
                              │ onSnapshot()
                    ┌─────────▼──────────┐
                    │   Web Dashboard    │
                    │  dashboard.html    │
                    │  attendance.html   │
                    └────────────────────┘
```
