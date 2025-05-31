import streamlit as st
import firebase_admin
from firebase_admin import credentials, firestore
from datetime import datetime, timedelta
import pandas as pd
import time

# Set Streamlit layout
st.set_page_config(page_title="Pill Consumption Tracker", layout="wide")
st.title("Pill Consumption Tracker")

# Initialize Firebase
if not firebase_admin._apps:
    cred = credentials.Certificate("medication-tracking-f24d7-firebase-adminsdk-fbsvc-a62de30b6e.json")
    firebase_admin.initialize_app(cred)

db = firestore.client()

def fetch_firebase_data():
    try:
        user_id = "u1234"
        sessions_ref = db.collection("users").document(user_id).collection("sessions")
        sessions = sessions_ref.stream()
        data = []
        for doc in sessions:
            entry = doc.to_dict()
            entry["session_id"] = doc.id
            data.append(entry)
        data.sort(key=lambda x: x.get("gesture_time", 0), reverse=True)
        return data
    except Exception as e:
        st.error(f"Failed to fetch Firebase data: {e}")
        return []

def display_status(latest_data):
    if not latest_data:
        st.warning("No pill consumption data available.")
        return

    gesture_time = latest_data.get("gesture_time", 0)
    time_ago = time.time() - gesture_time
    time_str = datetime.fromtimestamp(gesture_time).strftime("%Y-%m-%d %H:%M:%S")

    gesture_count = latest_data.get("gesture_count", 0)
    person_in_frame = latest_data.get("person_in_frame", False)
    session_valid = latest_data.get("session_valid", False)

    if latest_data.get("gesture_detected", False):
        st.success("✅ Pill was consumed!")
    else:
        st.error("❌ No pill consumption detected.")

    st.info(f"Detected {gesture_count} gesture(s) at {time_str}")
    st.info(f"Person in frame: {'Yes' if person_in_frame else 'No'}")
    st.info(f"Session valid: {'Yes' if session_valid else 'No'}")
    st.info(f"Time since: {timedelta(seconds=int(time_ago))}")

def display_history(data):
    if not data:
        st.info("No history to display.")
        return

    df = pd.DataFrame([{
        "Session": d.get("session_id", "n/a"),
        "Time": datetime.fromtimestamp(d["gesture_time"]).strftime("%Y-%m-%d %H:%M:%S"),
        "Gestures": d.get("gesture_count", 0),
        "Pill Consumed": "Yes" if d.get("gesture_detected") else "No",
        "In Frame": "Yes" if d.get("person_in_frame") else "No",
        "Valid": "Yes" if d.get("session_valid") else "No"
    } for d in data])

    st.subheader("Recent Pill Consumption History")
    st.dataframe(df, use_container_width=True)

def main():
    if st.button("Refresh"):
        all_data = fetch_firebase_data()
        if all_data:
            display_status(all_data[0])
            display_history(all_data)
        else:
            st.warning("No gesture data yet.")

    auto_refresh = st.checkbox("Auto-refresh every 10 seconds")
    if auto_refresh:
        while True:
            all_data = fetch_firebase_data()
            if all_data:
                display_status(all_data[0])
                display_history(all_data)
            else:
                st.warning("No gesture data yet.")
            time.sleep(10)
            st.rerun()

if __name__ == "__main__":
    main()