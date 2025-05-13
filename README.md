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

# Milestone 2
In this milestone, we completed the end-to-end system integration for our IoT-based pillbox. Using LDR sensors connected to an ESP32, we detect pill presence based on light intensity. The ESP32 processes sensor data with noise filtering, sends status updates to a FastAPI backend, and visualizes results through a Streamlit dashboard. This milestone focuses on achieving stable hardware-software communication and real-time pill status monitoring.

## Project Structure

- `device/` – Code for ESP32 microcontroller (PlatformIO)
- `backend/` – FastAPI server for receiving and storing pill data
- `frontend/` – Streamlit app to visualize pill statuses

## How to Run the Project

### 1. ESP32 Device Setup (PlatformIO)

#### Requirements
- VS Code with the PlatformIO extension
- ESP32 board
- Wi-Fi access

#### Steps
1. Clone this repository and open it in VS Code with PlatformIO.
2. Open the device code (usually `src/main.cpp`) and update your Wi-Fi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
3. Make sure your LDR sensors are connected to GPIO pins 4 through 10.
4. Connect the ESP32 board to your computer.
5. Upload the firmware:
   - Press the "Upload" button in PlatformIO toolbar, or run:
     ```
     pio run --target upload
     ```
6. Open the serial monitor to check output:
   ```
   pio device monitor
   ```

### 2. Backend Setup (FastAPI)

#### Requirements
- Python 3.9 or higher
- Install dependencies:
  ```
  pip install fastapi uvicorn
  ```

#### Run the server:
1. Navigate to the backend directory.
2. Run:
   ```
   uvicorn main:app --host 0.0.0.0 --port 8000
   ```
3. Confirm the backend is running at:
   ```
   http://<your-local-IP>:8000/api/pill-data
   ```

### 3. Frontend Setup (Streamlit)

#### Requirements
- Python 3.9 or higher
- Install dependencies:
  ```
  pip install streamlit requests pandas
  ```

#### Run the app:
1. Navigate to the frontend directory.
2. Run:
   ```
   streamlit run app.py
   ```
3. Open the browser window and click "Refresh Data" to view the latest pill box status.

