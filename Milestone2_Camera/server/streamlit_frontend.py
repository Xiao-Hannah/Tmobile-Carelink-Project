import streamlit as st
import requests
import pandas as pd
import time
from datetime import datetime, timedelta

st.set_page_config(page_title="Pill Consumption Tracker", layout="wide")
st.title("Pill Consumption Tracker")

# Set backend IP (now using Flask backend)
BACKEND_URL = "http://localhost:8001/api/gesture-data"  # Flask backend URL
LATEST_URL = "http://localhost:8001/api/latest-consumption"  # Flask backend URL

def fetch_data():
    try:
        response = requests.get(BACKEND_URL)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        st.error(f"Failed to fetch data: {e}")
        return []

def fetch_latest():
    try:
        response = requests.get(LATEST_URL)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        st.error(f"Failed to fetch latest data: {e}")
        return None

# Create a placeholder for the status indicator
status_placeholder = st.empty()

# Create a placeholder for historical data
history_placeholder = st.empty()

def display_status(latest_data):
    if not latest_data or "status" in latest_data:
        with status_placeholder.container():
            st.warning("No pill consumption data available yet.")
            return
            
    # Check if pill was consumed in last 24 hours
    time_ago = latest_data.get("time_ago", 0)
    pill_consumed = latest_data.get("pill_consumed", False)
    gesture_count = latest_data.get("gesture_count", 0)
    timestamp = latest_data.get("timestamp", 0)
    
    # Format time
    time_str = datetime.fromtimestamp(timestamp).strftime("%Y-%m-%d %H:%M:%S")
    
    with status_placeholder.container():
        if pill_consumed:
            st.success(f"✅ Pill was consumed!")
            st.info(f"Detected {gesture_count} hand-to-mouth gestures at {time_str}")
            
            # Calculate time since consumption
            hours_ago = time_ago / 3600
            if hours_ago < 24:
                st.info(f"Time since consumption: {timedelta(seconds=int(time_ago))}")
            else:
                st.warning(f"Time since consumption: {timedelta(seconds=int(time_ago))} (over 24 hours)")
        else:
            st.error(f"❌ No pill consumption detected")
            st.info(f"Last check: {time_str} ({gesture_count} gestures detected)")

def display_history(data):
    if not data:
        with history_placeholder.container():
            st.info("No historical data available.")
            return
    
    # Create a DataFrame for display
    records = []
    for item in data:
        records.append({
            "Time": datetime.fromtimestamp(item["timestamp"]).strftime("%Y-%m-%d %H:%M:%S"),
            "Gestures": item["gesture_count"],
            "Pill Consumed": "Yes" if item["pill_consumed"] else "No"
        })
    
    df = pd.DataFrame(records)
    
    with history_placeholder.container():
        st.subheader("Recent Pill Consumption History")
        st.dataframe(df, use_container_width=True)

def main():
    if st.button("Refresh Data"):
        latest_data = fetch_latest()
        display_status(latest_data)
        
        all_data = fetch_data()
        display_history(all_data)
    
    # Auto-refresh option
    auto_refresh = st.checkbox("Auto-refresh (every 10 seconds)")
    

    if auto_refresh:
        st.write("Auto-refreshing data...")
        latest_data = fetch_latest()
        display_status(latest_data)
        
        all_data = fetch_data()
        display_history(all_data)
        
        time.sleep(10)
        st.rerun()    
            

if __name__ == "__main__":
    main()