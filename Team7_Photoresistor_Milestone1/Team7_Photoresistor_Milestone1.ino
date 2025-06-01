const int ldrPin1 = 32; // First LDR on GPIO32
const int ldrPin2 = 33; // Second LDR on GPIO33
const int ldrPin3 = 12; // Third LDR on GPIO12
const int ldrPin4 = 35; // Fourth LDR on GPIO35
const int ldrPin5 = 34; // Fifth LDR on GPIO34
const int ldrPin6 = 13; // Sixth LDR on GPIO13
const int ldrPin7 = 14; // Seventh LDR on GPIO14

int ldrValue1 = 0;
int ldrValue2 = 0;
int ldrValue3 = 0;
int ldrValue4 = 0;
int ldrValue5 = 0;
int ldrValue6 = 0;
int ldrValue7 = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to start
  Serial.println("ESP32 Seven-LDR Test Starting...");
}

void loop() {
  // Print a message before reading to verify serial is working
  Serial.println("Reading sensors...");
  
  ldrValue1 = analogRead(ldrPin1); // Read the first LDR value
  ldrValue2 = analogRead(ldrPin2); // Read the second LDR value
  ldrValue3 = analogRead(ldrPin3); // Read the third LDR value
  ldrValue4 = analogRead(ldrPin4); // Read the fourth LDR value
  ldrValue5 = analogRead(ldrPin5); // Read the fifth LDR value
  ldrValue6 = analogRead(ldrPin6); // Read the sixth LDR value
  ldrValue7 = analogRead(ldrPin7); // Read the seventh LDR value
  
  Serial.print("LDR1 Value (pin 32): ");
  Serial.print(ldrValue1);
  Serial.print(" | LDR2 Value (pin 33): ");
  Serial.print(ldrValue2);
  Serial.print(" | LDR3 Value (pin 34): ");
  Serial.print(ldrValue3);
  Serial.print(" | LDR4 Value (pin 35): ");
  Serial.println(ldrValue4);
  
  Serial.print("LDR5 Value (pin 12): ");
  Serial.print(ldrValue5);
  Serial.print(" | LDR6 Value (pin 13): ");
  Serial.print(ldrValue6);
  Serial.print(" | LDR7 Value (pin 14): ");
  Serial.println(ldrValue7);

  // Threshold detection for LDR1
  if (ldrValue1 > 1600) {
    Serial.println("Pill removed from LDR1!");
  } else {
    Serial.println("Pill in place on LDR1.");
  }
  
  // Threshold detection for LDR2
  if (ldrValue2 > 1600) {
    Serial.println("Pill removed from LDR2!");
  } else {
    Serial.println("Pill in place on LDR2.");
  }
  
  // Threshold detection for LDR3
  if (ldrValue3 > 1000) {
    Serial.println("Pill removed from LDR3!");
  } else {
    Serial.println("Pill in place on LDR3.");
  }
  
  // Threshold detection for LDR4
  if (ldrValue4 > 1000) {
    Serial.println("Pill removed from LDR4!");
  } else {
    Serial.println("Pill in place on LDR4.");
  }
  
  // Threshold detection for LDR5
  if (ldrValue5 > 1500) {
    Serial.println("Pill removed from LDR5!");
  } else {
    Serial.println("Pill in place on LDR5.");
  }
  
  // Threshold detection for LDR6
  if (ldrValue6 > 1000) {
    Serial.println("Pill removed from LDR6!");
  } else {
    Serial.println("Pill in place on LDR6.");
  }
  
  // Threshold detection for LDR7
  if (ldrValue7 > 2300) {
    Serial.println("Pill removed from LDR7!");
  } else {
    Serial.println("Pill in place on LDR7.");
  }

  Serial.println("----------------------");
  delay(1000); // 1-second sampling
}
