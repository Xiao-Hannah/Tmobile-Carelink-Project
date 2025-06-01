// === Pill Monitor with Button Control and Firestore ===
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <algorithm>

// Button configuration
#define BUTTON_PIN 44

#define WIFI_SSID "TMOBILE-C883"
#define WIFI_PASSWORD "onion.bacteria.wreath.jury"
#define FIREBASE_API_KEY "AIzaSyDeKrdpCPBOX_QsZXdUlpRON2awf1RohaU"
#define FIREBASE_PROJECT_ID "medication-tracking-f24d7"
#define USER_EMAIL "yishuaiz@uw.edu"
#define USER_PASSWORD "123123123"

const int numLDRs = 7;
const int ldrPins[numLDRs] = {1, 2, 3, 4, 5, 6, 7};
float thresholds[numLDRs] = {400, 400, 400, 400, 400, 400, 300};
float emaValues[numLDRs];
bool pillPresent[numLDRs];

bool isOpen = false;  // true = open/detecting, false = closed/not detecting
String idToken = "";
unsigned long lastSend = 0;
const unsigned long sendInterval = 10000; // send every 10 seconds

bool loginToFirebase() {
  HTTPClient http;
  http.begin("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=" + String(FIREBASE_API_KEY));
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> doc;
  doc["email"] = USER_EMAIL;
  doc["password"] = USER_PASSWORD;
  doc["returnSecureToken"] = true;

  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  if (httpCode == 200) {
    String response = http.getString();
    StaticJsonDocument<1024> resDoc;
    deserializeJson(resDoc, response);
    idToken = resDoc["idToken"].as<String>();
    Serial.println("✅ Firebase login successful!");
    return true;
  } else {
    Serial.print("❌ Firebase login failed: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
    return false;
  }
}

float applyEMA(int i, float val, float alpha = 0.2) {
  emaValues[i] = alpha * val + (1 - alpha) * emaValues[i];
  return emaValues[i];
}

void sendToFirestore(int present, int removed) {
  if (idToken == "") {
    Serial.println("❌ Cannot send to Firestore — no idToken.");
    return;
  }

  HTTPClient http;
  String url = "https://firestore.googleapis.com/v1/projects/" + String(FIREBASE_PROJECT_ID) + "/databases/(default)/documents/users/u1234/pill_sessions";

  http.begin(url);
  http.addHeader("Authorization", "Bearer " + idToken);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<1024> doc;
  JsonObject fields = doc.createNestedObject("fields");
  fields["timestamp"]["integerValue"] = millis();
  fields["pills_present"]["integerValue"] = present;
  fields["pills_removed"]["integerValue"] = removed;

  String requestBody;
  serializeJson(doc, requestBody);

  int httpCode = http.POST(requestBody);
  if (httpCode == 200) {
    Serial.println("✅ Data sent to Firestore");
  } else {
    Serial.print("❌ Firestore write failed: ");
    Serial.println(httpCode);
    Serial.println(http.getString());
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  Serial.println("=== Pill Monitor with Button Control ===");
  Serial.println("Button: GPIO44");
  Serial.println("Press button to toggle OPEN/CLOSED");
  Serial.println();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");

  if (!loginToFirebase()) {
    Serial.println("❌ Could not authenticate with Firebase");
    return;
  }
}

void loop() {
  // Check button press (simple, no debouncing)
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);
  
  if (buttonState == LOW && lastButtonState == HIGH) {
    // Button just pressed
    isOpen = !isOpen;  // Toggle state
    Serial.println(isOpen ? "🟢 SYSTEM OPEN - Detecting pills" : "🔴 SYSTEM CLOSED - Not detecting");
    delay(200);  // Simple debounce
  }
  lastButtonState = buttonState;
  
  // Only detect pills and send data when system is OPEN
  if (isOpen) {
    unsigned long now = millis();
    if (now - lastSend >= sendInterval) {
      lastSend = now;

      int present = 0;
      Serial.println("--- Pill Detection ---");
      
      for (int i = 0; i < numLDRs; i++) {
        int val = analogRead(ldrPins[i]);
        float filtered = applyEMA(i, val);
        pillPresent[i] = filtered > thresholds[i];
        if (pillPresent[i]) present++;

        Serial.printf("Slot %d: Raw=%d, Filtered=%.1f, Status=%s\n",
                      i+1, val, filtered, pillPresent[i] ? "PILL" : "EMPTY");
      }

      Serial.printf("Result: %d pills present, %d empty\n", present, numLDRs - present);
      Serial.println();
      
      sendToFirestore(present, numLDRs - present);
    }
  }

  delay(50);
}