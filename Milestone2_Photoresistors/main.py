from fastapi import FastAPI
from pydantic import BaseModel
from typing import List
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# In-memory storage for pill data
pill_data_log = []

# Pydantic model for incoming data
class PillData(BaseModel):
    timestamp: int
    pill_status: List[bool]

@app.post("/api/pill-data")
async def receive_data(data: PillData):
    print("Received data:", data)
    pill_data_log.append(data)
    return {"status": "received"}

@app.get("/api/pill-data")
async def get_data():
    return pill_data_log[-20:]  # Return the last 20 entries
