#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Pins
#define BUTTON_PIN 44
#define LED_PIN 8        // White LED for refill
#define RED_LED_PIN 9    // Red LED for daily pill reminder

// WiFi
#define WIFI_SSID "TMOBILE-XXX" // replace with wifi ssid
#define WIFI_PASSWORD "XXXXXXXXXXXXXX" // replace with wifi password

// Server
const char* SERVER_URL = "http://XXX.XXX.XX.XXX:8000/ldr-data"; // flask server url, replace and KEEP ":8000/ldr-data"

// LDR setup
const int numLDRs = 7;
const int ldrPins[numLDRs] = {1, 2, 3, 4, 5, 7, 6};
const float removalThreshold = 400.0;

// Refill alert config
const int REFILL_THRESHOLD = 2;
unsigned long refillStartTime = 0;
const unsigned long SIMULATED_DAY_MS = 60000;
const int SIMULATED_REFILL_DAY = 6;

// Daily reminder config
const unsigned long SIMULATED_DAY_DURATION = 60000;   // 1 min = 1 day
const unsigned long SIMULATED_8AM_MS = 8000;          // 8 sec = 8AM
bool pillTakenToday = false;

bool isOpen = false;
unsigned long sessionTimestamp = 0;

float openStartReadings[numLDRs];
float openEndReadings[numLDRs];
unsigned long lidOpenStartTime = 0;
const unsigned long openCaptureDelay = 1000;  // Capture baseline 1s after open
bool openStartCaptured = false;
bool openEndCaptured = false;

float applyEMA(float prev, float val, float alpha = 0.2) {
  return alpha * val + (1 - alpha) * prev;
}
float rollingEndReadings[numLDRs];
unsigned long lastRollingUpdateTime = 0;
const unsigned long rollingUpdateInterval = 100;   // update every 100 ms

const int rollingBufferSize = 20; // ~2 seconds if updated every 100ms
float rollingEndHistory[7][rollingBufferSize];
unsigned long rollingTimestamps[rollingBufferSize];
int rollingIndex = 0;
const unsigned long minDeltaBeforeClose = 500; // 0.5 seconds before close for snapshot

void flashRefillLED() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(LED_PIN, LOW);
    delay(300);
  }
}

void sendToServer(int present, int removed, String removedSlotsCSV) {
  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(256);
  doc["timestamp"] = sessionTimestamp;
  // doc["pills_present"] = present; - innacurate, use removed instead
  doc["pills_removed"] = removed;
  doc["removed_slots"] = removedSlotsCSV;

  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  if (httpCode == 200) {
    Serial.println("✅ LDR data sent to server");
  } else {
    Serial.print("❌ Failed to send data: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
  }

  http.end();
}

bool checkNeedsRefill() {
  unsigned long elapsed = millis() - refillStartTime;
  int simulatedDay = (elapsed / SIMULATED_DAY_MS) % 7;

  Serial.printf("🕒 Simulated Day: %d\n", simulatedDay);

  // Only return true if it's Sunday
  return (simulatedDay == SIMULATED_REFILL_DAY);
}

void handleDailyReminder() {
  unsigned long dayTime = (millis() - refillStartTime) % SIMULATED_DAY_DURATION;

  if (dayTime >= SIMULATED_8AM_MS && !pillTakenToday) {
    digitalWrite(RED_LED_PIN, HIGH);
  } else if (pillTakenToday) {
    digitalWrite(RED_LED_PIN, LOW);
  }

  // Reset pillTakenToday flag at midnight (new simulated day)
  static int lastSimDay = -1;
  int currentSimDay = (millis() - refillStartTime) / SIMULATED_DAY_DURATION;

  if (currentSimDay != lastSimDay) {
    Serial.printf("🌅 New Simulated Day: %d\n", currentSimDay);
    pillTakenToday = false;
    lastSimDay = currentSimDay;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  Serial.println("=== Pill Monitor with Refill + Reminder ===");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < 1700000000) {
    delay(200);
    Serial.println("⌛ Waiting for NTP time sync...");
  }
  Serial.printf("✅ Time synced! Epoch: %lu\n", time(nullptr));

  refillStartTime = millis();  // Start simulated clock
}

void loop() {
  handleDailyReminder();

  static bool lastLidState = HIGH;
  bool currentLidState = digitalRead(BUTTON_PIN);

  // --- Lid just opened ---
  if (currentLidState == HIGH && lastLidState == LOW) {
    isOpen = true;
    Serial.println("🟢 Lid OPEN — Capturing baseline...");

    lidOpenStartTime = millis();
    openStartCaptured = false;
    sessionTimestamp = time(nullptr);
    Serial.printf("🆕 Session started — Timestamp: %lu\n", sessionTimestamp);

    // Clear history
    for (int i = 0; i < rollingBufferSize; i++) {
      rollingTimestamps[i] = 0;
      for (int j = 0; j < numLDRs; j++) {
        rollingEndHistory[j][i] = 0;
      }
    }
    rollingIndex = 0;
  }

  // --- Lid just closed ---
  else if (currentLidState == LOW && lastLidState == HIGH) {
    isOpen = false;
    unsigned long lidCloseTime = millis();
    unsigned long lidDuration = lidCloseTime - lidOpenStartTime;

    if (lidDuration < 1000) {
      Serial.printf("⚠️ Lid opened and closed too quickly (%.0f ms) — skipping analysis.\n", (float)lidDuration);
      
      // 🔴 Blink red LED briefly to show invalid capture
      digitalWrite(RED_LED_PIN, HIGH);
      delay(300);
      digitalWrite(RED_LED_PIN, LOW);

      lastLidState = currentLidState;
      return;
    }

    Serial.println("🔴 Lid CLOSED — Analyzing changes");

    int bestIndex = -1;

    for (int i = 0; i < rollingBufferSize; i++) {
      if (lidCloseTime - rollingTimestamps[i] >= minDeltaBeforeClose) {
        bestIndex = i;
      }
    }

    if (bestIndex != -1) {
      for (int i = 0; i < numLDRs; i++) {
        openEndReadings[i] = rollingEndHistory[i][bestIndex];
      }
      Serial.printf("📸 Selected end snapshot from %.0f ms before lid close.\n", lidCloseTime - rollingTimestamps[bestIndex]);
    } else {
      Serial.println("⚠️ No stable end snapshot found — using latest.");
      for (int i = 0; i < numLDRs; i++) {
        openEndReadings[i] = rollingEndHistory[i][rollingIndex];
      }
    }

    int present = 0, removed = 0;
    String removedSlotsCSV = "";

    for (int i = 0; i < numLDRs; i++) {
      float diff = openStartReadings[i] - openEndReadings[i];
      bool removedHere = diff > removalThreshold;

      Serial.printf("Slot %d | Start=%.1f -> End=%.1f | Δ=%.1f | Removed: %s\n",
                    i + 1, openStartReadings[i], openEndReadings[i], diff,
                    removedHere ? "YES" : "NO");

      if (removedHere) {
        removed++;
        if (removedSlotsCSV != "") removedSlotsCSV += ",";
        removedSlotsCSV += String(i + 1);
      } else {
        present++;
      }
    }

    Serial.printf("📤 Sending result — Removed: %d\n", removed);
    sendToServer(present, removed, removedSlotsCSV);
    pillTakenToday = true;  // Mark pill taken for today

    if (checkNeedsRefill()) {
      Serial.println("⚠️ Refill needed — flashing LED");
      flashRefillLED();
    } else {
      Serial.println("✅ No refill needed");
    }
    openStartCaptured = false;
  }

  // --- While lid is open ---
  if (isOpen) {
    unsigned long timeOpen = millis() - lidOpenStartTime;

    if (!openStartCaptured && timeOpen >= openCaptureDelay) {
      for (int i = 0; i < numLDRs; i++) {
        openStartReadings[i] = analogRead(ldrPins[i]);
      }
      openStartCaptured = true;
      Serial.println("📸 Captured early baseline while lid open.");
    }

    if (millis() - lastRollingUpdateTime >= rollingUpdateInterval) {
      for (int i = 0; i < numLDRs; i++) {
        rollingEndHistory[i][rollingIndex] = analogRead(ldrPins[i]);
      }
      rollingTimestamps[rollingIndex] = millis();
      rollingIndex = (rollingIndex + 1) % rollingBufferSize;
      lastRollingUpdateTime = millis();
    }
  }

  lastLidState = currentLidState;
  delay(20);
}
