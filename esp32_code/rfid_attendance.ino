/*
 * ============================================================
 *  RFID Smart Attendance System — ESP32 Firmware
 *  Hardware: ESP32 + RC522 RFID + LCD I2C + Buzzer + LEDs
 *  Author: IoT Attendance Project
 *  Version: 1.0
 * ============================================================
 *
 *  LIBRARY INSTALLATION (Arduino IDE → Tools → Manage Libraries):
 *  1. "MFRC522" by GithubCommunity
 *  2. "LiquidCrystal I2C" by Frank de Brabander
 *  3. "ArduinoJson" by Benoit Blanchon
 *
 *  WIRING:
 *  ─── RC522 ──────────────────────
 *  RC522 SDA  → GPIO 5
 *  RC522 SCK  → GPIO 18
 *  RC522 MOSI → GPIO 23
 *  RC522 MISO → GPIO 19
 *  RC522 RST  → GPIO 22
 *  RC522 3.3V → 3.3V
 *  RC522 GND  → GND
 *
 *  ─── LCD I2C (16x2) ─────────────
 *  LCD SDA    → GPIO 21
 *  LCD SCL    → GPIO 22   ← SHARE with RC522 RST? No!
 *  NOTE: Use GPIO 4 for LCD SCL if GPIO 22 used for RFID RST
 *  Recommended: LCD SCL → GPIO 4, LCD SDA → GPIO 21
 *
 *  ─── LEDs ───────────────────────
 *  Green LED  → GPIO 2  (+ 220Ω → GND)
 *  Red LED    → GPIO 15 (+ 220Ω → GND)
 *
 *  ─── Buzzer ─────────────────────
 *  Buzzer +   → GPIO 13
 *  Buzzer -   → GND
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ─── WiFi Credentials ──────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ─── Firebase Config ────────────────────────────────────────
// Get from Firebase Console → Project Settings → Service Accounts
// Or use the REST API with your Web API Key
const char* FIREBASE_HOST    = "YOUR_PROJECT_ID.firebaseio.com";  // Realtime DB
const char* FIREBASE_API_KEY = "YOUR_WEB_API_KEY";
const char* FIREBASE_PROJECT  = "YOUR_PROJECT_ID";

// For Firestore REST API
// URL format: https://firestore.googleapis.com/v1/projects/{project}/databases/(default)/documents/{collection}
String FIRESTORE_URL = "https://firestore.googleapis.com/v1/projects/" + String(FIREBASE_PROJECT) + "/databases/(default)/documents/attendance";
String STUDENTS_URL  = "https://firestore.googleapis.com/v1/projects/" + String(FIREBASE_PROJECT) + "/databases/(default)/documents/students";

// ─── Google Apps Script URL (for Google Sheet backup) ───────
const char* GOOGLE_SHEET_URL = "YOUR_GOOGLE_APPS_SCRIPT_URL";
// Replace with your deployed Apps Script Web App URL

// ─── Pin Definitions ────────────────────────────────────────
#define SS_PIN     5     // RC522 SDA/SS
#define RST_PIN    22    // RC522 RST
#define LED_GREEN  2     // Green LED
#define LED_RED    15    // Red LED
#define BUZZER_PIN 13    // Buzzer

// ─── Objects ────────────────────────────────────────────────
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address 0x27 or 0x3F

// ─── Duplicate Scan Prevention ───────────────────────────────
struct RecentScan {
  String uid;
  unsigned long scanTime;
};
const int MAX_RECENT = 20;
const unsigned long COOLDOWN_MS = 30000;  // 30 seconds cooldown
RecentScan recentScans[MAX_RECENT];
int recentCount = 0;

// ─── Firebase Auth Token ─────────────────────────────────────
String firebaseToken = "";

// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== RFID Attendance System ===");
  Serial.println("Initializing…");

  // GPIO Setup
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED,   OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED,   LOW);

  // LCD Setup
  Wire.begin(21, 4);  // SDA=21, SCL=4 (adjust if needed)
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID Attendance");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // SPI & RC522 Setup
  SPI.begin();
  rfid.PCD_Init();
  delay(500);
  Serial.println("[RFID] RC522 initialized");

  // Connect WiFi
  connectWiFi();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  lcd.setCursor(0, 1);
  lcd.print("Scan your card");

  Serial.println("[SYS] Ready — Waiting for RFID card…");
  beepSuccess();
}

// ============================================================
void loop() {
  // Reconnect WiFi if dropped
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Reconnecting…");
    connectWiFi();
  }

  // Look for RFID card
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Read UID
  String uid = readUID();
  Serial.println("[RFID] Card detected: " + uid);

  // Duplicate check
  if (isDuplicateScan(uid)) {
    Serial.println("[RFID] Duplicate scan — ignoring (cooldown active)");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Already Marked!");
    lcd.setCursor(0, 1);
    lcd.print(uid);
    beepDuplicate();
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("System Ready!");
    lcd.setCursor(0, 1);
    lcd.print("Scan your card");
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  // Show scanning status
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Checking...");
  lcd.setCursor(0, 1);
  lcd.print(uid);

  // Look up student in Firestore
  StudentInfo student = lookupStudent(uid);

  if (student.found) {
    // Mark attendance
    String now    = getTimeString();
    String today  = getDateString();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Welcome!");
    lcd.setCursor(0, 1);
    // Truncate name to 16 chars
    String dispName = student.name.length() > 16 ? student.name.substring(0, 16) : student.name;
    lcd.print(dispName);

    Serial.println("[STUDENT] Name: "  + student.name);
    Serial.println("[STUDENT] ID: "    + student.studentId);
    Serial.println("[STUDENT] Dept: "  + student.department);

    // Build attendance record
    String jsonPayload = buildAttendanceJSON(student, uid, now, today);

    // Send to Firebase Firestore
    bool firebaseSent = sendToFirestore(jsonPayload);

    // Send to Google Sheet (optional backup)
    bool sheetSent = sendToGoogleSheet(student.name, uid, now, today);

    // Visual/audio feedback
    if (firebaseSent) {
      beepSuccess();
      flashLED(LED_GREEN, 3);
      addToRecentScans(uid);

      delay(2000);
      lcd.setCursor(0, 0);
      lcd.print("Attendance Saved");
      lcd.setCursor(0, 1);
      lcd.print(now + " OK");
      Serial.println("[OK] Attendance recorded for " + student.name);
    } else {
      beepError();
      flashLED(LED_RED, 3);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Send Failed!");
      lcd.setCursor(0, 1);
      lcd.print("Check Network");
      Serial.println("[ERR] Failed to send attendance");
    }

  } else {
    // Unknown card
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unknown Card!");
    lcd.setCursor(0, 1);
    lcd.print(uid);

    Serial.println("[ERR] UID not found in database: " + uid);
    beepError();
    flashLED(LED_RED, 2);
  }

  delay(3000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  lcd.setCursor(0, 1);
  lcd.print("Scan your card");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ============================================================
// WiFi Connection
// ============================================================
void connectWiFi() {
  Serial.print("[WiFi] Connecting to " + String(WIFI_SSID));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(attempts % 16, 1);
    lcd.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected! IP: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    delay(1500);
  } else {
    Serial.println("\n[WiFi] Connection FAILED — running offline");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Offline mode");
    delay(1500);
  }
}

// ============================================================
// Read RFID UID as HEX string
// ============================================================
String readUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

// ============================================================
// Student Lookup (Firestore REST API)
// ============================================================
struct StudentInfo {
  bool   found;
  String name;
  String studentId;
  String department;
  String email;
};

StudentInfo lookupStudent(String uid) {
  StudentInfo s = {false, "", "", "", ""};

  if (WiFi.status() != WL_CONNECTED) return s;

  // Firestore document URL: /students/{uid}
  String url = STUDENTS_URL + "/" + uid + "?key=" + FIREBASE_API_KEY;

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.GET();

  Serial.println("[HTTP] Student lookup: " + String(code));

  if (code == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    // Parse Firestore document fields
    if (doc.containsKey("fields")) {
      JsonObject fields = doc["fields"];
      s.found      = true;
      s.name       = fields["name"]["stringValue"]       | "Unknown";
      s.studentId  = fields["studentId"]["stringValue"]  | "";
      s.department = fields["department"]["stringValue"] | "General";
      s.email      = fields["email"]["stringValue"]      | "";
    }
  }

  http.end();
  return s;
}

// ============================================================
// Build Attendance JSON for Firestore
// ============================================================
String buildAttendanceJSON(StudentInfo& s, String uid, String time, String date) {
  /*
    Firestore REST API expects this format:
    {
      "fields": {
        "name":       {"stringValue": "..."},
        "studentId":  {"stringValue": "..."},
        ...
      }
    }
  */
  String json = "{\"fields\":{";
  json += "\"name\":{\"stringValue\":\"" + s.name + "\"},";
  json += "\"studentId\":{\"stringValue\":\"" + s.studentId + "\"},";
  json += "\"uid\":{\"stringValue\":\"" + uid + "\"},";
  json += "\"department\":{\"stringValue\":\"" + s.department + "\"},";
  json += "\"status\":{\"stringValue\":\"Present\"},";
  json += "\"time\":{\"stringValue\":\"" + time + "\"},";
  json += "\"date\":{\"stringValue\":\"" + date + "\"}";
  json += "}}";
  return json;
}

// ============================================================
// Send attendance to Firebase Firestore
// ============================================================
bool sendToFirestore(String jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = FIRESTORE_URL + "?key=" + FIREBASE_API_KEY;

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(jsonPayload);
  Serial.println("[Firestore] Response: " + String(code));

  if (code > 0) {
    Serial.println("[Firestore] Body: " + http.getString());
  }

  http.end();
  return (code == 200 || code == 201);
}

// ============================================================
// Send to Google Sheets via Apps Script
// ============================================================
bool sendToGoogleSheet(String name, String uid, String time, String date) {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (String(GOOGLE_SHEET_URL).startsWith("YOUR_")) return false;

  String url = String(GOOGLE_SHEET_URL) +
    "?name=" + urlEncode(name) +
    "&uid=" + uid +
    "&time=" + urlEncode(time) +
    "&date=" + date +
    "&status=Present";

  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int code = http.GET();

  Serial.println("[GSheet] Response: " + String(code));
  http.end();
  return (code == 200);
}

// ============================================================
// Duplicate scan check
// ============================================================
bool isDuplicateScan(String uid) {
  unsigned long now = millis();
  for (int i = 0; i < recentCount; i++) {
    if (recentScans[i].uid == uid) {
      if ((now - recentScans[i].scanTime) < COOLDOWN_MS) {
        return true;  // Still in cooldown
      } else {
        // Expired — remove it
        recentScans[i] = recentScans[--recentCount];
        return false;
      }
    }
  }
  return false;
}

void addToRecentScans(String uid) {
  if (recentCount < MAX_RECENT) {
    recentScans[recentCount++] = {uid, millis()};
  } else {
    // Overwrite oldest
    recentScans[0] = {uid, millis()};
  }
}

// ============================================================
// Time/Date helpers (using millis as fallback)
// For accurate time, use NTP:
//   #include <time.h>
//   configTime(21600, 0, "pool.ntp.org");  // UTC+6 for Bangladesh
// ============================================================
String getTimeString() {
  // Use NTP for real time; this is a placeholder
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "00:00 AM";
  }
  char buf[10];
  strftime(buf, sizeof(buf), "%I:%M %p", &timeinfo);
  return String(buf);
}

String getDateString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "0000-00-00";
  }
  char buf[12];
  strftime(buf, sizeof(buf), "%Y-%m-%d", &timeinfo);
  return String(buf);
}

void setupNTP() {
  // Bangladesh time: UTC+6
  configTime(6 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("[NTP] Syncing time");
  struct tm timeinfo;
  int retries = 0;
  while (!getLocalTime(&timeinfo) && retries++ < 10) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n[NTP] Time synced: " + getTimeString());
}

// ============================================================
// URL encode (for Google Sheets GET params)
// ============================================================
String urlEncode(String str) {
  String encoded = "";
  for (int i = 0; i < str.length(); i++) {
    char c = str[i];
    if (c == ' ') encoded += "%20";
    else if (isAlphaNumeric(c)) encoded += c;
    else { encoded += "%"; if (c < 0x10) encoded += "0"; encoded += String(c, HEX); }
  }
  return encoded;
}

// ============================================================
// LED & Buzzer helpers
// ============================================================
void flashLED(int pin, int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH); delay(150);
    digitalWrite(pin, LOW);  delay(100);
  }
}

void beepSuccess() {
  tone(BUZZER_PIN, 1000, 150);
  delay(200);
  tone(BUZZER_PIN, 1500, 150);
  delay(200);
  noTone(BUZZER_PIN);
}

void beepError() {
  tone(BUZZER_PIN, 300, 500);
  delay(600);
  noTone(BUZZER_PIN);
}

void beepDuplicate() {
  tone(BUZZER_PIN, 800, 100);
  delay(150);
  tone(BUZZER_PIN, 800, 100);
  delay(200);
  noTone(BUZZER_PIN);
}
