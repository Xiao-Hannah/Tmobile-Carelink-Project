// === Pill Monitor with Refill Alert and Daily Pill Reminder ===
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
#define WIFI_SSID "TMOBILE-C883"
#define WIFI_PASSWORD "onion.bacteria.wreath.jury"

// #define WIFI_SSID "UW MPSK"
// #define WIFI_PASSWORD "9hpR?/3#^*"

// Server
// const char* SERVER_URL = "http://10.19.197.81:8000/ldr-data";

const char* SERVER_URL = "http://192.168.12.150:8000/ldr-data";

// LDR setup
const int numLDRs = 7;
const int ldrPins[numLDRs] = {1, 2, 3, 4, 5, 7, 6};
float slotThresholds[numLDRs] = {1800, 2200, 1000, 800, 1500, 950, 2000};
float baselineReadings[numLDRs];
float finalReadings[numLDRs];
bool pillWasPresent[numLDRs];

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
unsigned long lastRead = 0;
const unsigned long readInterval = 1000;
unsigned long sessionTimestamp = 0;

float applyEMA(float prev, float val, float alpha = 0.2) {
  return alpha * val + (1 - alpha) * prev;
}

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
  doc["pills_present"] = present;
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

bool checkNeedsRefill(int filledSlots) {
  unsigned long elapsed = millis() - refillStartTime;
  int simulatedDay = (elapsed / SIMULATED_DAY_MS) % 7;
  Serial.printf("🕒 Simulated Day: %d | Filled Slots: %d\n", simulatedDay, filledSlots);

  bool isSunday = (simulatedDay == SIMULATED_REFILL_DAY);
  return isSunday || (!isSunday && filledSlots == 0);
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

  if (currentLidState != lastLidState) {
    isOpen = (currentLidState == HIGH);
    Serial.println(isOpen ? "🟢 Lid OPEN — Capturing baseline..." : "🔴 Lid CLOSED — Analyzing changes");

    if (isOpen) {
      pillTakenToday = true;
      sessionTimestamp = time(nullptr);
      Serial.printf("🆕 Session started — Timestamp: %lu\n", sessionTimestamp);

      for (int i = 0; i < numLDRs; i++) {
        int val = analogRead(ldrPins[i]);
        baselineReadings[i] = applyEMA(val, val);
        pillWasPresent[i] = baselineReadings[i] > slotThresholds[i];
      }

    } else {
      int present = 0, removed = 0;

      for (int i = 0; i < numLDRs; i++) {
        int val = analogRead(ldrPins[i]);
        finalReadings[i] = applyEMA(baselineReadings[i], val);
        bool removedHere = pillWasPresent[i] && (finalReadings[i] < baselineReadings[i] - 200);
        if (!pillWasPresent[i]) continue;

        if (removedHere) removed++;
        else present++;

        Serial.printf("Slot %d | Base=%.1f -> Final=%.1f | Removed: %s\n", i + 1,
                      baselineReadings[i], finalReadings[i], removedHere ? "YES" : "NO");
      }

      Serial.printf("📤 Sending result — Present: %d | Removed: %d\n", present, removed);
      int mostLikelySlot = -1;
      float maxDiff = 0.0;

      for (int i = 0; i < numLDRs; i++) {
        if (pillWasPresent[i]) {
          float diff = baselineReadings[i] - finalReadings[i];
          if (diff > 200 && diff > maxDiff) {
            maxDiff = diff;
            mostLikelySlot = i;
          }
        }
      }

      String removedSlotsCSV = "";
      if (mostLikelySlot != -1) {
        removedSlotsCSV = String(mostLikelySlot + 1);  // 1-indexed slot
      }

      sendToServer(present, removed, removedSlotsCSV);

      if (checkNeedsRefill(present)) {
        Serial.println("⚠️ Refill needed — flashing LED");
        flashRefillLED();
      } else {
        Serial.println("✅ No refill needed");
      }
    }

    delay(200);  // Debounce
  }

  lastLidState = currentLidState;
  delay(20);
}

