# SmartDrive_Score
Code for ESP32‐based CAN sender &amp; receiver + Python scoring script (YOLOv8, MQTT, PDF report)
# Automated Driving Evaluation Robot

This repository hosts the complete code and documentation for an automated driving evaluation robot that replaces a human examiner with an objective, connected system. The project was developed at École Nationale des Sciences Appliquées – Marrakech, Université Cadi Ayyad.
---

## Introduction

Obtaining a driver’s license traditionally involves a human examiner assessing a candidate’s ability to follow traffic rules an approach that can be subjective and constrained by examiner availability. This project implements a robotic platform that objectively evaluates “driving” performance by combining two ESP32 boards, a CAN bus network, an MQTT link, and a YOLOv8-based Python script to score the robot’s compliance with traffic signs and lane boundaries in real time. 

---

## System Architecture

The system consists of three tightly integrated components:

1. **ESP32 Master (Car_Controller_and_CAN_SENDER)**  
   - Controlled via a Blynk mobile app over Wi-Fi to drive the robot (forward, backward, left, right) and adjust speed.  
   - Reads four infrared (IR) line-tracking sensors to detect the black road line and white edges.  
   - Toggles two LEDs (simulating turn signals).  
   - Packages speed, sensor states, and LED states into CAN frames (IDs 0x01–0x09) at 500 kbps and transmits them to the slave ESP32. 

2. **ESP32 Slave (CAN_Receiver_and_Mqtt)**  
   - Listens on the CAN bus for incoming frames from the master.  
   - Parses each payload and republishes it to an MQTT broker under topics `esp32/v1` … `esp32/v8`.  
   - Ensures the Python scoring script can access real-time telemetry (speed, IR sensors, LEDs) via MQTT. 

3. **Python Scoring Script (score_model_code/detect_signs.py)**  
   - Subscribes to MQTT topics to receive live sensor and speed data.  
   - Captures frames from a webcam (or phone camera via IP Webcam) and runs YOLOv8 (trained on custom traffic-sign images) to detect and classify road signs (Stop, speed-limit 60 km/h, speed-limit 40 km/h, turn-only, etc.).  
   - Applies predefined scoring rules:  
     - Deducts points if the robot fails to stop at a Stop sign or improperly adjusts speed upon speed-limit signs.  
     - Checks whether the correct turn signal LED is activated when a turn-only sign appears.  
     - Validates that the robot remains on the road line (IR sensor data) or deducts points for lane departure.  
   - Displays the annotated video feed and continuously prints the running score.  
   - (Future extension) Generates a PDF report with driver name, CIN, final score, and comments. 

---

## Components & Pinouts

### ESP32 Master (Car_Controller_and_CAN_SENDER)

- **IR Line-Tracking Sensors**  
  - IR_SENSOR_RIGHT1 → GPIO 2  
  - IR_SENSOR_LEFT1 → GPIO 5  
  - IR_SENSOR_RIGHT2 → GPIO 4  
  - IR_SENSOR_LEFT2 → GPIO 23

- **Motor Driver (H-Bridge)**  
  - ENA → GPIO 25  
  - IN1 → GPIO 26  
  - IN2 → GPIO 27  
  - IN3 → GPIO 12  
  - IN4 → GPIO 14  
  - ENB → GPIO 13

- **Turn Signal LEDs**  
  - LED → GPIO 33  
  - LED1 → GPIO 32

- **CAN Transceiver (TJA1050)**  
  - TX (ESP32) → GPIO 17 → TXD on TJA1050  
  - RX (ESP32) → GPIO 16 → RXD on TJA1050  
  - CANH/CANL connect to the CAN bus lines

- **Blynk Virtual Pins**  
  - V1 → Forward button  
  - V2 → Right turn button  
  - V3 → Left turn button  
  - V4 → Backward button  
  - V5 → Speed slider (0–255)  
  - V6 → LED1 toggle (left turn signal)  
  - V7 → LED2 toggle (right turn signal) :contentReference[oaicite:0]{index=0}

### ESP32 Slave (CAN_Receiver_and_Mqtt)

- **CAN Transceiver (TJA1050)**  
  - TX (ESP32) → GPIO 17  
  - RX (ESP32) → GPIO 16  
- **MQTT Topics**  
  - `esp32/v1` … `esp32/v8` map directly to CAN IDs 0x01 … 0x09 . 

### Python Scoring Script (score_model_code)

- **Webcam Input**  
  - Uses OpenCV to capture from `cv2.VideoCapture(0)` or IP Webcam URL.
- **YOLOv8 Model**  
  - Weights file (`west.pt`) trained on custom traffic-sign dataset (Stop, speed limit 60/40, turn signs, pedestrian crossing, etc.).  
  - Model inference threshold set to 0.8 confidence. 
- **MQTT Client**  
  - Subscribes to `esp32/v1` … `esp32/v8` via `paho-mqtt`.  
  - Stores the latest integer payload for each topic in a dictionary for scoring logic.

---
Licensing
This project is released under the MIT License. See the LICENSE file for details.

Acknowledgments
We gratefully acknowledge the contributions of the entire project team (El Forkani Anas,El Hazdour Ahmed, El Bouaissaoui Bouchra, El Baz Ahlam, El Hatimy Maryam, El Goumri Hiba, El Kharraz Naoual) .
