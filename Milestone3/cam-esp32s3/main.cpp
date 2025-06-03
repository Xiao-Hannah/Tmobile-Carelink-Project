#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include "camera_pins.h"
#include <time.h> 


// const char* serverUrl = "http://10.19.227.222:8000/upload";  // Flask server IP

const char* ssid = "TMOBILE-C883";
const char* password = "onion.bacteria.wreath.jury";
const char* serverUrl = "http://192.168.12.150:8000/cam";  // Flask server IP

// Button configuration
#define BUTTON_PIN 2         // GPIO 2 for lid switch
#define FRAMES_PER_CAPTURE 50  // Number of frames to capture
#define CAPTURE_DELAY 3000   // 3-second delay before capturing
unsigned long sessionTimestamp = 0;

// SPDT switch configuration - change these to variables instead of #define
bool usingNOTerminal = true;   // Set to true if using Normally Open terminal
bool usingNCTerminal = false;  // Set to true if using Normally Closed terminal

// Camera pin definitions (unchanged)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

// State variables for lid detection
bool lidWasClosed = true;      // Assume lid starts closed
unsigned long lidOpenTime = 0;
bool captureScheduled = false;
bool captureInProgress = false;

void startCamera() {
  // Camera initialization (unchanged)
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QQVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed with error 0x%x", err);
    while (true);  // halt
  } else {
    Serial.println("✅ Camera initialized");
  }
}

// Helper function to check if lid is closed based on SPDT wiring
bool isLidClosed() {
  int buttonState = digitalRead(BUTTON_PIN);
  
  if (usingNOTerminal) {
    // When using NO terminal connected to GND:
    // Button pressed -> LOW -> Lid closed
    // Button released -> HIGH -> Lid open
    return (buttonState == LOW);
  } 
  else if (usingNCTerminal) {
    // When using NC terminal connected to GND:
    // Button pressed -> HIGH -> Lid closed
    // Button released -> LOW -> Lid open
    return (buttonState == HIGH);
  }
  
  // Default case (fallback to NO terminal behavior)
  return (buttonState == LOW);
}

void captureFrames() {
  const int frames_to_capture = FRAMES_PER_CAPTURE;
  int frame_count = 0;

  int heapBefore = ESP.getFreeHeap();
  unsigned long start = millis();
  unsigned long captureStart = millis();

  while (frame_count < frames_to_capture) {
    if (millis() - captureStart > 10000) {
      Serial.println("⏳ Max capture duration reached. Stopping.");
      break;
    }
    if (isLidClosed()) {
      Serial.println("⚠️ Lid closed during capture - stopping");
      break;
    }

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) continue;

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String urlWithTS = String(serverUrl) + "?ts=" + String(sessionTimestamp);
      http.begin(urlWithTS);
      http.addHeader("Content-Type", "image/jpeg");
      int httpResponseCode = http.POST(fb->buf, fb->len);
      Serial.printf("📤 Frame %d sent — status: %d\n", frame_count + 1, httpResponseCode);
      http.end();
    }

    esp_camera_fb_return(fb);
    delay(100);  // wait between frames
    frame_count++;
  }

  // 🔔 AFTER sending all frames, send the burst completion signal
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String doneUrl = String(serverUrl) + "-done?ts=" + String(sessionTimestamp);
    http.begin(doneUrl);
    int doneResponseCode = http.GET();
    Serial.printf("📩 Done signal sent — status: %d\n", doneResponseCode);
    http.end();
  }

  unsigned long end = millis();
  int heapAfter = ESP.getFreeHeap();

  Serial.printf("🧠 RAM used during capture: %d bytes\n", heapBefore - heapAfter);
  Serial.printf("⏱️ Total capture time: %lu ms\n", end - start);
  Serial.println("💤 Done capturing burst.");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize lid switch (button) pin with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Print SPDT configuration
  Serial.println("\n🔌 SPDT Switch Configuration:");
  if (usingNOTerminal) {
    Serial.println("   Using Normally Open (NO) terminal");
    Serial.println("   Connect: COMMON → GND, NO → GPIO 2");
  } else if (usingNCTerminal) {
    Serial.println("   Using Normally Closed (NC) terminal");
    Serial.println("   Connect: COMMON → GND, NC → GPIO 2");
  }
  
  // Check initial lid state
  bool lidClosed = isLidClosed();
  Serial.println(lidClosed ? "📦 Lid is closed" : "📂 Lid is open");
  lidWasClosed = lidClosed;

  Serial.println("📶 Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;

    if (attempts >= 30) {
      Serial.println("\n⚠️ Still not connected. Retrying WiFi...");
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(ssid, password);
      attempts = 0;
    }
  }

  Serial.println("\n✅ WiFi connected!");
  Serial.print("📍 ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < 1700000000) {
    delay(200);
    Serial.println("⌛ Waiting for NTP time sync...");
  }
  Serial.printf("✅ Time synced! Epoch: %lu\n", time(nullptr));

  startCamera();
  
  Serial.println("\n✨ System ready");
  Serial.println("   Open the lid to trigger capture after 3 seconds");
}

void loop() {
  // Read lid state using our helper function
  bool lidClosed = isLidClosed();
  
  // Debug output (remove in final version)
  static unsigned long lastDebugTime = 0;
  if (millis() - lastDebugTime > 5000) {  // Every 5 seconds
    lastDebugTime = millis();
    Serial.printf("🔍 Button state: %d | Interpreted as: %s | Scheduled: %s\n", 
                 digitalRead(BUTTON_PIN),
                 lidClosed ? "CLOSED" : "OPEN",
                 captureScheduled ? "YES" : "NO");
  }
  
  // Detect when lid changes from closed to open
  if (lidClosed) {
    // Lid is closed
    lidWasClosed = true;
    captureScheduled = false;  // Cancel any scheduled capture if lid closes
  } 
  else if (lidWasClosed) {
    lidWasClosed = false;
    lidOpenTime = millis();
    sessionTimestamp = time(nullptr);  // 🕒 Store timestamp for syncing session
    Serial.printf("📂 Lid opened - will capture in 3 seconds | Session TS: %lu\n", sessionTimestamp);
    captureScheduled = true;
  }
  
  // Check if it's time to start capture after lid opening
  if (captureScheduled && !lidClosed && !captureInProgress && 
      millis() - lidOpenTime >= CAPTURE_DELAY) {
    Serial.println("⏱️ Delay complete - starting capture");
    captureInProgress = true;
    captureFrames();
    captureInProgress = false;
    captureScheduled = false;
    Serial.println("✅ Capture complete. Close and reopen lid to repeat.");
  }
  
  // Process serial commands
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "status") {
      // Report status
      Serial.println("\n📊 STATUS REPORT");
      Serial.printf("   WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
      Serial.printf("   IP: %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("   Button state (raw): %d\n", digitalRead(BUTTON_PIN));
      Serial.printf("   Lid: %s\n", lidClosed ? "Closed" : "Open");
      Serial.printf("   Switch type: SPDT using %s terminal\n", 
                   usingNOTerminal ? "NO (Normally Open)" : "NC (Normally Closed)");
      Serial.printf("   Server: %s\n", serverUrl);
      Serial.printf("   Frames per capture: %d\n", FRAMES_PER_CAPTURE);
      Serial.printf("   Capture delay: %d ms\n", CAPTURE_DELAY);
      Serial.println("\n✨ Ready for operation");
    }
    else if (command == "togglemode") {
      // For testing - toggle between NO and NC mode
      if (usingNOTerminal) {
        usingNOTerminal = false;
        usingNCTerminal = true;
        Serial.println("✅ Switched to NC (Normally Closed) terminal mode");
      } else {
        usingNOTerminal = true;
        usingNCTerminal = false;
        Serial.println("✅ Switched to NO (Normally Open) terminal mode");
      }
    }
    else if (command == "test") {
      // Run a test capture regardless of button state
      Serial.println("🔍 TEST: Starting capture sequence");
      captureFrames();
      Serial.println("🔍 TEST: Capture complete");
    }
  }
  
  delay(100);  // Small delay to prevent CPU hogging
}
