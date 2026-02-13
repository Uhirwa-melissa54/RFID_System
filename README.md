# RFID Top-Up System

## Project Overview
This project implements a complete RFID card top-up system using an ESP8266, MQTT, Flask backend, and a web dashboard.  

**Features:**
- Users can scan RFID cards using the ESP8266 + MFRC522.
- Card balances are maintained in real-time.
- Top-up requests are sent from a browser dashboard to the backend.
- Real-time balance updates are pushed to the web dashboard using WebSocket.

## Live Web Dashboard
Access the dashboard here:  

http://157.173.101.159:9254/dash.html
## Repository Structure
TOP_UP_PROJECT/
│
├─ backend/
│ ├─ main.py # Flask + SocketIO backend
│ ├─ requirements.txt # Python dependencies
│ └─ public/
│ └─ index.html # Web Dashboard front-end
├─ top_up/
│ └─ top_up.ino # ESP8266 / Arduino firmware
└─ README.md # Project overview and instructions

##TeamMate: IKIREZI UNEJUMUTIMA Honorine
