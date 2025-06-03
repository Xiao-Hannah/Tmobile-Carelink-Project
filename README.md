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

This is the same structure we're using for the camera component as well, detecting the gesture using MediaPipe library on the server side code. The distance between the hand and the nose indicates whether the pill-taking gesture has been made, and the frontend visualizes key information including time of camera activated, number of gestures made and whether we can make the assumption that the pill has been consumed.

## Photoresistor Setup (ESP32S3 + LDR + FastAPI + Streamlit)

This device uses an **ESP32 microcontroller** and **photoresistors (LDRs)** to monitor the status of pills being present or removed in pillbox. It includes a **FastAPI backend** to collect and store real-time sensor data, and a **Streamlit frontend** to visualize values from pill-taking events.

## 🔧 Hardware Setup

### Components
- ESP32S3
- 7 × photoresistors (LDR)
- 7 × 10kΩ resistors
- Battery and power switch
- PCB
- Wi-Fi connection

### Wiring
Each LDR is connected in a voltage divider circuit with a 10kΩ resistor. Connect the analog output of each divider to the following ESP32 pins:
- LDR1 → GPIO 4
- LDR2 → GPIO 5
- LDR3 → GPIO 6
- LDR4 → GPIO 7
- LDR5 → GPIO 8
- LDR6 → GPIO 9
- LDR7 → GPIO 10

## Project Structure
smart-pillbox/
│
├── arduino/ # ESP32 code (PlatformIO)
│ └── main.cpp
│
├── backend/ # FastAPI server
│ └── main.py
│
├── frontend/ # Streamlit dashboard
│ └── app.py
│
└── README.md


---

## Setup Instructions

### 1. ESP32 Firmware

#### Prerequisites
- **PlatformIO IDE** (or VSCode with PlatformIO extension)
- **ESP32 board support** (installed via PlatformIO)
- Libraries:
  - `WiFi.h`
  - `HTTPClient.h`
  - `ArduinoJson`

#### Upload Steps
1. Open `src/main.cpp` in PlatformIO IDE.
2. Set your **Wi-Fi credentials**:
   ```cpp
   const char* ssid = "YOUR_SSID";
   const char* password = "YOUR_PASSWORD";

3. Set your backend IP:
const char* serverUrl = "http://10.18.101.245:8000/api/pill-data";
4. Select board: ESP32 Dev Module.
5. Upload the code and open the Serial Monitor to verify connectivity.


### 2. FastAPI Backend
#### Prerequisites
- Python 3.9+
pip install fastapi uvicorn
- Run
cd backend
uvicorn main:app --host 0.0.0.0 --port 8000
The backend listens at: http://10.18.101.245:8000/api/pill-data
It stores and serves recent pill data in-memory.

### 3. Streamlit Frontend
#### Prerequisites
- Python 3.9+
- Install Streamlit and other dependencies:
pip install streamlit pandas requests

#### Run
1. Navigate to the frontend folder.
2. Start the Streamlit frontend:
streamlit run app.py
The frontend will be available on the local machine and will display recent pill status data. You can refresh the data by clicking the "Refresh Data" button.

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

## Camera Setup (ESP32S3 + Attached Camera + Server.py + FastAPI + Streamlit)

## 🔧 Hardware Setup

### Components
- ESP32S3
- Camera attachment
- Battery and power switch
- Button (GPIO2)
- Wi-Fi connection

#### Upload Steps
1. Open `src/main.cpp` in PlatformIO IDE.
2. Set your **Wi-Fi credentials**:
   ```cpp
   const char* ssid = "YOUR_SSID";
   const char* password = "YOUR_PASSWORD";
   const char* serverUrl = "URL";
   ```
3. Upload the code and open the Serial Monitor to verify connectivity.

#### Server Side Steps
1. Navigate to `server` folder from terminal.
2. Open `flask_server.py`, `flask_backend.py` and `streamlit_frontend.py`.
3. Set up a virtual environment with the following sequence (note - please ensure python 3.10 has been installed - this is necessary for some packages required)
   ```terminal
   rm -rf venv
   python3.10 -m venv venv
   source venv/bin/activate
   pip install --no-deps --no-cache-dir -r requirements.txt
   ```
4. To add your own firebase, remember to add your credentials JSON file to the same `server` folder, and reference it in the `flask_server.py` code.
5. Open three terminals, each running each script.
6. Start by running the `flask_backend.py` with `python flask_backend.py` in terminal.
7. Run `flask_server.py` with `python flask_server.py` in terminal.
8. Run `streamlit_frontend.py` with `streamlit run streamlit_frontend.py`.
9. Navigate to local host, turn on your camera device and start capturing the gestures by pressing the button!


# Milestone 3
In this final milestone, we focused on integrating both photoresistor and camera subsystems into a unified medication tracking platform, supporting real-time doctor-patient interactions. 

## Full System Integration
We deployed two complementary sensing modalities:
- Photoresistors (LDRs) detect physical removal of pills from each compartment.
- Camera Module captures and verifies pill-taking gestures via MediaPipe (hand-to-nose movement).
  
Data from both sensors are timestamped and sent to a cloud-based FastAPI/Flask backend. If both LDR and gesture detection occur within a defined time window, the system confidently marks the pill as “taken.” Otherwise, it flags the action as “Not Taken” on both patient and doctor portal, and the LED light on the pill box will flash red to notify. If either the photoresistor or the camera fails to detect correctly, the system marks the status as “Uncertain,” which there is a chance to allow the patinet/caregive to manually confirm medication intake status on the patient portal. After a 7-day period, the LED on the pillbox will blink white to remind the patient/caregiver to refill the pillbox.

## Deployment
- All components are housed in a custom PCB and a 3D-printed enclosure, powered via battery.
- The system (The camera and the photo resistors) is automatically activated when the pillbox is opened, triggered by an embedded switch mechanism placed inside the bottom left part of the pill box.
- The device connects to the cloud through T-Mobile’s 5G network, which supports real-time data upload and remote monitoring without depending on local Wi-Fi.
- Backend supports real-time Firebase integration, enabling potential EHR sync in the future.

## Frontend Visualization
We interated on and finalized the UI design for both patient and doctor dashboards and implemented responsive dashboards using React, HTML, CSS, and JavaScript within the Cursor development environment to visualize and receiving real-time data collection from the pillbox. We also integrated our portals with two other teams sponsored by the T-Mobile CareLink+ project, allowing both patients and doctors to sign in through one unified platform.

#### Key features include:
##### Daily Pill Intake Tracking: 
- Displays pill status by time and compartment using a simple, color-coded calendar view.
##### Weekly Adherence Summaries:
- Visualizes weekly medication patterns using backend-generated data and intuitive charts.
##### Blood Pressure Summary:
- Provides a quick overview of the patient’s recent blood pressure readings for contextual health insights.
##### Medication Adherence Reminders:
- Automatically sends reminders to patients based on missed doses or irregular intake patterns.
##### Doctor-to-Patient Messaging:
- Allows doctors to send personalized notes or feedback directly to patients, which can be reviewed in their individual portal.



