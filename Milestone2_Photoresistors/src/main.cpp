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

void setup() {
  Serial.begin(115200);
  delay(1000);

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
