# T-mobile-Carelink-Project
A project to develop a system that allows doctors to remotely verify patient adherence to hypertension medication.

# Milestone 1
At this stage, we used seven photoresistors (LDRs) to detect the presence or removal of pills in a custom pill dispenser system. The code reads analog values from each sensor and prints out a real-time status message over the serial monitor.
## Photoresistor Pill Detection 
- Monitors 7 individual LDRs for light level changes.
- Determines whether a pill is in place or has been removed.
- Prints live data to the Serial Monitor at 1-second intervals.

### Hardware Requirements
1 × ESP32-WROOM Development Board
7 × Photoresistors (LDRs)
7 × 7k Ohm Resistors (for voltage divider circuits)
Breadboard and jumper wires
USB cable to connect ESP32-WROOM to your computer

### Circuit Diagram
Each LDR must be connected in a voltage divider circuit with a 10kΩ resistor. Connect the analog output of each divider to the following ESP32 pins:
- LDR1 → GPIO 32
- LDR2 → GPIO 33
- LDR3 → GPIO 12
- LDR4 → GPIO 35
- LDR5 → GPIO 34
- LDR6 → GPIO 13
- LDR7 → GPIO 14

### Software Setup
1.	Install Arduino IDE
Download from: https://www.arduino.cc/en/software
2.	Add ESP32 Board Support
Open File > Preferences
Go to Tools > Board > Boards Manager, search for ESP32 WROOM, and install it
3. Select the Correct Board
Tools > Board > ESP32 Dev Module (or your specific board)
Tools > Port > Select the appropriate COM port

### Upload & Run
1.	Clone or copy this code into a new .ino file in Arduino IDE.
2.	Click Upload to flash the code to your ESP32-WROOM.
3.	Open the Serial Monitor (Tools > Serial Monitor) and set baud rate to 115200.
4.	Observe the output for LDR readings and pill status.
