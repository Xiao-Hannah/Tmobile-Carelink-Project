<<<<<<< HEAD
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

bool isDetecting = false;  // true when actively detecting for 10s
unsigned long detectionStartTime = 0;
const unsigned long detectionDuration = 10000; // 10 seconds
String idToken = "";

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
=======
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// LDR analog input pins (adjust as per your wiring)
const int ldrPin1 = 4, ldrPin2 = 5, ldrPin3 = 6, ldrPin4 = 7, ldrPin5 = 8, ldrPin6 = 9, ldrPin7 = 10;

// Wi-Fi credentials
const char* ssid = "UW MPSK";
const char* password = "/7je7r&;JW";

// Backend URL
const String backendUrl = "http://10.18.101.245:8000/api/pill-data";

// Raw and filtered values
int ldrValues[7];
int filteredValues[7];

// Median filter window size
const int WINDOW_SIZE = 5;
int readings[7][WINDOW_SIZE];
int readIndex = 0;

// EMA filter alpha
float alpha = 0.3;
float prevFiltered[7] = {0, 0, 0, 0, 0, 0, 0};

// Thresholds for determining status
const int thresholds[7] = {600, 600, 1000, 1000, 500, 1000, 2300};
>>>>>>> 9ec079eede6be46a9898273f33073074de04877f

void setup() {
  Serial.begin(115200);
  delay(1000);
<<<<<<< HEAD
  
  // Initialize button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  Serial.println("=== Pill Monitor with Button Control ===");
  Serial.println("Button: GPIO44");
  Serial.println("Press and release button to start 10s detection");
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
  // Check button press to release (box opening detection)
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);
  
  if (buttonState == HIGH && lastButtonState == LOW) {
    // Button just released (box opened)
    isDetecting = true;
    detectionStartTime = millis();
    Serial.println("🟢 BOX OPENED - Starting 10s detection...");
  }
  lastButtonState = buttonState;
  
  // Handle 10-second detection period
  if (isDetecting) {
    unsigned long elapsed = millis() - detectionStartTime;
    
    if (elapsed >= detectionDuration) {
      // 10 seconds completed - take final reading and upload
      isDetecting = false;
      
      int present = 0;
      Serial.println("--- Final Pill Detection ---");
      
      for (int i = 0; i < numLDRs; i++) {
        int val = analogRead(ldrPins[i]);
        float filtered = applyEMA(i, val);
        pillPresent[i] = filtered > thresholds[i];
        if (pillPresent[i]) present++;

        Serial.printf("Slot %d: Raw=%d, Filtered=%.1f, Status=%s\n",
                      i+1, val, filtered, pillPresent[i] ? "PRESENT" : "REMOVED");
      }

      Serial.printf("Result: %d pills present, %d pills removed\n", present, numLDRs - present);
      Serial.println();
      
      sendToFirestore(present, numLDRs - present);
      Serial.println("🔴 Detection complete - Ready for next box opening");
      Serial.println();
    } else {
      // Show countdown during detection
      static unsigned long lastCountdown = 0;
      if (millis() - lastCountdown >= 1000) {
        lastCountdown = millis();
        int remaining = (detectionDuration - elapsed) / 1000 + 1;
        Serial.printf("Detecting... %ds remaining\n", remaining);
      }
    }
  }

  delay(50);
}
=======

  // Initialize LDR readings
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < WINDOW_SIZE; j++) {
      readings[i][j] = 0;
    }
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Ensure stable Wi-Fi connection
  delay(2000);
}

int medianFilter(int newValue, int* readings) {
  readings[readIndex] = newValue;
  int temp[WINDOW_SIZE];
  for (int i = 0; i < WINDOW_SIZE; i++) temp[i] = readings[i];
  // Sort the readings array
  for (int i = 0; i < WINDOW_SIZE - 1; i++) {
    for (int j = 0; j < WINDOW_SIZE - i - 1; j++) {
      if (temp[j] > temp[j + 1]) {
        int t = temp[j]; temp[j] = temp[j + 1]; temp[j + 1] = t;
      }
    }
  }
  return temp[WINDOW_SIZE / 2];
}

int emaFilter(int newValue, float* prevFiltered) {
  float filtered = alpha * newValue + (1 - alpha) * (*prevFiltered);
  *prevFiltered = filtered;
  return (int)filtered;
}

void sendDataToBackend() {
  HTTPClient http;
  String payload = "{\"timestamp\": " + String(millis()) + ", \"pill_status\": [";

  for (int i = 0; i < 7; i++) {
    payload += (filteredValues[i] > thresholds[i]) ? "true," : "false,";
  }

  payload.remove(payload.length() - 1); // Remove trailing comma
  payload += "]}";

  // ✅ Print the payload before sending
  Serial.println("Sending payload: " + payload);

  http.begin(backendUrl);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.println("Data sent successfully!");
  } else {
    Serial.println("Error sending data: " + String(httpResponseCode));
  }
  http.end();
}

void loop() {
  // Read LDR values
  for (int i = 0; i < 7; i++) {
    ldrValues[i] = analogRead(i + 4); // LDR pin mapping (adjust based on actual pins)
  }

  // Apply median filter to each LDR
  for (int i = 0; i < 7; i++) {
    int medianValue = medianFilter(ldrValues[i], readings[i]);
    filteredValues[i] = emaFilter(medianValue, &prevFiltered[i]);
  }

  // ✅ Print filtered LDR values
  Serial.print("Filtered LDR values: [");
  for (int i = 0; i < 7; i++) {
    Serial.print(filteredValues[i]);
    if (i < 6) Serial.print(", ");
  }
  Serial.println("]");

  // Send data to backend
  sendDataToBackend();

  // Delay before next reading
  delay(5000);
}
>>>>>>> 9ec079eede6be46a9898273f33073074de04877f
