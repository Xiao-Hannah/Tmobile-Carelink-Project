import streamlit as st
import requests
import pandas as pd
from datetime import datetime

st.set_page_config(page_title="Pill Box Status Viewer", layout="wide")
st.title("Pill Box Status Viewer")

# Set backend IP
BACKEND_URL = "http://10.18.101.245:8000/api/pill-data"

# Function to fetch data from the backend
def fetch_data():
    try:
        response = requests.get(BACKEND_URL)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        st.error(f"Failed to fetch data: {e}")
        return []

# Function to convert timestamp to a readable format
def format_timestamp(timestamp):
    return datetime.utcfromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')

# Streamlit button to fetch data
if st.button("Refresh Data"):
    data = fetch_data()

    if data:
        latest = data[-1]  # Get the most recent data

        # Get pill status and timestamp
        pill_status = latest.get("pill_status", [])
        timestamp = latest.get("timestamp", "N/A")

        # Format the timestamp if it's an integer (Unix timestamp)
        if isinstance(timestamp, int):
            timestamp = format_timestamp(timestamp)
        
        # Create a DataFrame for display
        df = pd.DataFrame({
            "LDR": [f"LDR{i+1}" for i in range(len(pill_status))],
            "Status": ["Pill removed" if status else "Pill present" for status in pill_status],
        })

        # Display the most recent reading
        st.subheader(f"Latest Reading — Timestamp: {timestamp}")
        st.dataframe(df, use_container_width=True)
    else:
        st.warning("No data received yet.")
