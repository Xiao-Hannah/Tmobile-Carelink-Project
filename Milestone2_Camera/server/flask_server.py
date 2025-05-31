
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
import threading
import firebase_admin
from firebase_admin import credentials, firestore

# Initialize Firebase once
cred = credentials.Certificate("--------.json")  # Path to your downloaded JSON -fill in the name of that file
firebase_admin.initialize_app(cred)
db = firestore.client()


frame_buffer = []
landmarks_buffer = []
poses_detected = []
last_detection_time = 0
USER_ID = "u1234"
processing_in_progress = False

app = Flask(__name__)

def log_to_backends(data):
    # Send to FastAPI
    try:
        response = requests.post(FASTAPI_URL, json=data)
        if response.status_code == 200:
            print(f"✅ Sent to UI backend: {data}")
        else:
            print(f"❌ FastAPI failed: {response.status_code}")
    except Exception as e:
        print(f"❌ FastAPI error: {e}")

    # Function to get session ID based on time# Note: Firebase logging to 'pill_gestures' collection is deprecated
    # All gesture logs now go through log_gesture() for session-based Firestore tracking
def get_session_id_from_time(timestamp):
    dt = datetime.fromtimestamp(timestamp)
    hour = dt.hour
    if hour < 12:
        return "morning"
    elif hour < 18:
        return "afternoon"
    else:
        return "evening"

def get_current_date(timestamp):
    return datetime.fromtimestamp(timestamp).strftime("%Y-%m-%d")

def log_gesture(user_id, timestamp, gesture_count, person_in_frame, session_valid):    
    session_type = get_session_id_from_time(timestamp)
    session_id = f"{session_type}_{timestamp}"  # e.g. "morning_1716549632"
    doc_ref = db.collection("users").document(user_id).collection("sessions").document(session_id)
    doc_ref.set({
        "session_type": session_type,
        "gesture_detected": gesture_count > 0,
        "gesture_count": gesture_count,
        "person_in_frame": person_in_frame,
        "session_valid": session_valid,
        "gesture_time": timestamp,
        "device_id": "esp32-cam",
        "timestamp": timestamp,
        "last_updated": firestore.SERVER_TIMESTAMP
    }, merge=True)

def clear_buffers():
    frame_buffer.clear()
    landmarks_buffer.clear()
    poses_detected.clear()
    if hasattr(receive_image, 'burst_start_time'):
        receive_image.burst_start_time = datetime.now()

# Background worker to monitor timeout
def timeout_watcher():
    global last_detection_time, processing_in_progress
    user_id = "u1234"
    while True:
        time.sleep(1)

        if not processing_in_progress and len(landmarks_buffer) > 0:
            time_since_last = (datetime.now() - receive_image.last_frame_time).total_seconds()
            time_since_start = (datetime.now() - receive_image.burst_start_time).total_seconds()

            if time_since_last > 2.5 or time_since_start > 8:
                # Prevent multiple processing
                processing_in_progress = True

                if time.time() - last_detection_time < 10:
                    print("⚠️ Skipping session — detected too soon after previous one (throttled)")
                    clear_buffers()
                    processing_in_progress = False
                    continue

                last_detection_time = time.time()

                if len(frame_buffer) < 5:
                    print("⚠️ Too few frames, skipping session")
                    clear_buffers()
                    processing_in_progress = False
                    continue
                # Process gesture session
                count = analyze_gestures(landmarks_buffer)
                valid_pose_frames = sum(1 for pose in poses_detected if pose)
                min_pose_ratio = 0.7
                person_in_frame = (valid_pose_frames / len(frame_buffer)) >= min_pose_ratio
                gesture_detected = count > 0
                session_valid = person_in_frame and gesture_detected

                print(f"🤚 DETECTED {count} PILL TAKING GESTURE(S)")
                print(f"ℹ️ Processed {len(landmarks_buffer)} frames with poses out of {len(frame_buffer)} total")

                try:
                    timestamp = int(time.time())
                    now = datetime.fromtimestamp(timestamp)
                    data = {
                        "timestamp": timestamp,
                        "gesture_count": count,
                        "pill_consumed": gesture_detected,
                        "readable_time": now.strftime("%Y-%m-%d %H:%M:%S")
                    }
                    log_to_backends(data)
                    log_gesture(USER_ID, timestamp, count, person_in_frame, session_valid)
                except Exception as e:
                    print(f"❌ Error sending to UI backend: {e}")

                clear_buffers()
                processing_in_progress = False

# FastAPI backend URL
FASTAPI_URL = "http://localhost:8001/api/gesture-data" 

mp_pose = mp.solutions.pose
pose = mp_pose.Pose(static_image_mode=False, model_complexity=0) 

@app.route('/upload', methods=['POST'])
def receive_image():
    global frame_buffer, landmarks_buffer, poses_detected

    # Track burst start
    if not hasattr(receive_image, 'burst_start_time'):
        receive_image.burst_start_time = datetime.now()

    # Reset burst start if this is the first frame
    if len(frame_buffer) == 0:
        receive_image.burst_start_time = datetime.now()

    # Read incoming image
    img_data = np.frombuffer(request.data, np.uint8)
    frame = cv2.imdecode(img_data, cv2.IMREAD_COLOR)

    if frame is not None:
        # Store frame
        frame_buffer.append(frame)
        image_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        results = pose.process(image_rgb)

        # Update time of last received frame
        receive_image.last_frame_time = datetime.now()

        if results.pose_landmarks:
            poses_detected.append(True)
            print("✅ Pose detected.")
            keypoints = [[lm.x, lm.y, lm.z] for lm in results.pose_landmarks.landmark]
            landmarks_buffer.append(keypoints)
        else:
            poses_detected.append(False)
            print("❌ Pose NOT detected.")
    else:
        print("⚠️ Failed to decode frame")

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
    # Start background timeout checker
    threading.Thread(target=timeout_watcher, daemon=True).start()
    app.run(host="0.0.0.0", port=8000)
