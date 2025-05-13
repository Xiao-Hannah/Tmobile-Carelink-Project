
# # 🧠 Hand-to-Mouth Gesture Detection (ESP32S3 Wi-Fi Input)

# # This script receives camera frames from an ESP32S3 via HTTP POST,
# # runs MediaPipe pose detection, computes hand-to-nose distance,
# # and detects hand-to-mouth gestures based on smoothed distance signals.

# ------------------------------------------------------------------------------------------------------------------------------- #

import cv2
import numpy as np
import os
from flask import Flask, request
from datetime import datetime
from scipy.signal import savgol_filter, find_peaks
import mediapipe as mp
import requests
import time

app = Flask(__name__)
frame_buffer = []
landmarks_buffer = []

# FastAPI backend URL
FASTAPI_URL = "http://localhost:8001/api/gesture-data" 

mp_pose = mp.solutions.pose
pose = mp_pose.Pose(static_image_mode=False, model_complexity=0) 

@app.route('/upload', methods=['POST'])
def receive_image():
    global frame_buffer, landmarks_buffer
    
    # Initialize last_frame_time if this is the first frame
    if not hasattr(receive_image, 'last_frame_time'):
        receive_image.last_frame_time = datetime.now()
    
    # Process incoming frame
    img_data = np.frombuffer(request.data, np.uint8)
    frame = cv2.imdecode(img_data, cv2.IMREAD_COLOR)

    if frame is not None:
        # Store frame and detect pose
        frame_buffer.append(frame)
        image_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = pose.process(image_rgb)

        # Update the last frame time
        receive_image.last_frame_time = datetime.now()

        if results.pose_landmarks:
            print("✅ Pose detected.")
            keypoints = [[lm.x, lm.y, lm.z] for lm in results.pose_landmarks.landmark]
            landmarks_buffer.append(keypoints)
        else:
            print("❌ Pose NOT detected.")
    
        # Analysis trigger conditions
        time_since_last_frame = (datetime.now() - receive_image.last_frame_time).total_seconds()
        
        if (len(landmarks_buffer) >= 50 or  # Enough landmarks 
            len(frame_buffer) >= 50 or      # All frames received
            (len(landmarks_buffer) > 0 and time_since_last_frame > 1.0)):  # Timed out
            
            if len(landmarks_buffer) > 0:
                # Analyze gestures
                count = analyze_gestures(landmarks_buffer)
                print(f"🤚 DETECTED {count} PILL TAKING GESTURE(S)")
                print(f"ℹ️ Processed {len(landmarks_buffer)} frames with poses out of {len(frame_buffer)} total")
                
                # Send to FastAPI backend
                try:
                    pill_consumed = count >= 1  # Logic: if gesture count ≥ 1, pill was consumed
                    data = {
                        "timestamp": int(time.time()),
                        "gesture_count": count,
                        "pill_consumed": pill_consumed
                    }
                    response = requests.post(FASTAPI_URL, json=data)
                    if response.status_code == 200:
                        print(f"✅ Sent to UI backend: {count} gestures, pill consumed: {pill_consumed}")
                    else:
                        print(f"❌ Failed to send to UI backend: {response.status_code}")
                except Exception as e:
                    print(f"❌ Error sending to UI backend: {e}")
                
                # Clear buffers
                frame_buffer.clear()
                landmarks_buffer.clear()
                return str(count), 200
            else:
                print("⚠️ No valid poses detected in the entire sequence")
                frame_buffer.clear()
                return "0", 200  # No gestures detected

    return "OK", 200

def analyze_gestures(landmarks_buffer):
    landmarks = np.array(landmarks_buffer)

    distances = []
    for frame in landmarks:
        hand = np.mean([frame[15][:2], frame[16][:2]], axis=0)
        nose = frame[0][:2]
        dist = np.linalg.norm(hand - nose)
        distances.append(dist)

    L = len(distances)
    window_length = min(11, L if L % 2 == 1 else L - 1)

    if window_length >= 3:
        smoothed = savgol_filter(distances, window_length=window_length, polyorder=2)
    else:
        smoothed = distances  # not enough points to smooth

    inverted = -np.array(smoothed)
    peaks, _ = find_peaks(inverted, height=-0.7, distance=10, prominence=0.1)

    return len(peaks)

if __name__ == "__main__":
    print("🚀 Starting Flask server...")
    app.run(host="0.0.0.0", port=8000)