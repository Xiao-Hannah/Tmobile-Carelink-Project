from flask import Flask, request, jsonify
import time

app = Flask(__name__)

# In-memory storage for gesture detection events
gesture_log = []

@app.route('/')
def read_root():
    return jsonify({"status": "Pill Gesture Backend is running"})

@app.route('/api/gesture-data', methods=['POST'])
def receive_gesture():
    data = request.json
    print(f"Received gesture data: {data}")
    gesture_log.append(data)
    return jsonify({"status": "received"})

@app.route('/api/gesture-data', methods=['GET'])
def get_gesture_data():
    # Return the most recent 20 records
    return jsonify(gesture_log[-20:] if gesture_log else [])

@app.route('/api/latest-consumption', methods=['GET'])
def get_latest_consumption():
    if not gesture_log:
        return jsonify({"status": "No data available"})
    
    latest = gesture_log[-1]
    return jsonify({
        "timestamp": latest.get("timestamp", 0),
        "gesture_count": latest.get("gesture_count", 0),
        "pill_consumed": latest.get("pill_consumed", False),
        "time_ago": time.time() - latest.get("timestamp", 0)
    })

@app.after_request
def after_request(response):
    response.headers.add('Access-Control-Allow-Origin', '*')
    response.headers.add('Access-Control-Allow-Headers', 'Content-Type,Authorization')
    response.headers.add('Access-Control-Allow-Methods', 'GET,PUT,POST,DELETE')
    return response

if __name__ == "__main__":
    print("Starting Flask backend server on port 8001...")
    app.run(host="0.0.0.0", port=8001, debug=True)