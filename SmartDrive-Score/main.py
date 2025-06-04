import cv2
from ultralytics import YOLO
import paho.mqtt.client as mqtt
import time

# -----------------------------------------------------------------------------
#                        Configuration & Constants
# -----------------------------------------------------------------------------
MODEL_PATH = r'C:\model\path\Mymodel.pt'  # Replace with your model path
MQTT_BROKER = "mqtt.eclipseprojects.io"
MQTT_PORT = 1883

FRAME_SKIP_INTERVAL = 10
DETECTION_THRESHOLD = 0.8

# Initial score
score = 100.0

# -----------------------------------------------------------------------------
#                        MQTT Topics & Data Storage
# -----------------------------------------------------------------------------
MQTT_TOPICS = {
    "esp32/v1": "v1",  # Speed value (0–255), actual speed = int(payload) - 100
    "esp32/v2": "v2",  # LED/sensor flags
    "esp32/v3": "v3",
    "esp32/v4": "v4",  # e.g. stop‐sign flag
    "esp32/v5": "v5",  # e.g. speed‐limit flag
    "esp32/v6": "v6",  # Turn signal flag
    "esp32/v7": "v7",  # Additional flag
    "esp32/v8": "v8",  # Reserved or other flag
}

# Store the most recent integer payload for each topic
mqtt_data = {key: 0 for key in MQTT_TOPICS.values()}

# -----------------------------------------------------------------------------
#                          MQTT Callback Functions
# -----------------------------------------------------------------------------
def on_connect(client, userdata, flags, rc):
    print(f"Connected to MQTT broker with result code {rc}")
    for topic in MQTT_TOPICS:
        client.subscribe(topic)


def on_message(client, userdata, msg):
    """Store incoming payloads as integers in mqtt_data dict."""
    key = MQTT_TOPICS.get(msg.topic)
    if key is not None:
        try:
            mqtt_data[key] = int(msg.payload.decode())
            print(f"Received {msg.topic}: {mqtt_data[key]}")
        except ValueError:
            print(f"Warning: Non‐numeric payload for {msg.topic}: {msg.payload.decode()}")


# -----------------------------------------------------------------------------
#                  Helper: Calculate Speed Penalty Function
# -----------------------------------------------------------------------------
def speed_penalty(raw_speed):
    """
    raw_speed: int from 0–255 published on 'esp32/v1'
    The code subtracts 100 to get actual speed.
    Returns penalty based on the resulting speed.
    """
    actual_speed = raw_speed - 100
    if 40 <= actual_speed <= 80:
        return 0.2
    if 81 <= actual_speed <= 150:
        return 0.8
    if actual_speed >= 151:
        return 2.0
    return 0.0


# -----------------------------------------------------------------------------
#                   Helper: Check Required LED/Sensor Flags
# -----------------------------------------------------------------------------
def check_required_flags():
    """
    Required flags: v3, v4, v5, v6 must all be 1 (e.g., stop sign, speed-limit, turn signal).
    Returns True if all are 1, False otherwise.
    """
    return (
        mqtt_data['v3'] == 1 and
        mqtt_data['v4'] == 1 and
        mqtt_data['v5'] == 1 and
        mqtt_data['v6'] == 1
    )


# -----------------------------------------------------------------------------
#                             Main Script
# -----------------------------------------------------------------------------
def main():
    global score

    # Initialize webcam
    cap = cv2.VideoCapture(0) #you can place your Camer Ip 
    if not cap.isOpened():
        print("Error: Could not open webcam.")
        return

    # Load YOLOv8 model
    model = YOLO(MODEL_PATH)

    # Set up MQTT client
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
    client.loop_start()

    frame_count = 0
    current_flag = 0  # Holds class_id for additional rules

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("Error: Failed to read frame.")
                break

            # Only run detection every FRAME_SKIP_INTERVAL frames
            if frame_count % FRAME_SKIP_INTERVAL == 0:
                results = model(frame)[0]
                detections = results.boxes.data.tolist()  # [[x1, y1, x2, y2, conf, class_id], ...]

                if not detections:
                    # No detections: check required flags
                    if not check_required_flags():
                        score -= 0.4
                else:
                    # For each detection above threshold:
                    for box in detections:
                        x1, y1, x2, y2, conf, class_id = box
                        if conf < DETECTION_THRESHOLD:
                            continue

                        # Draw bounding box & label
                        class_name = results.names[int(class_id)]
                        cv2.rectangle(frame,
                                      (int(x1), int(y1)),
                                      (int(x2), int(y2)),
                                      (0, 255, 0), 2)
                        cv2.putText(frame,
                                    f"{class_name} ({conf:.2f})",
                                    (int(x1), int(y1 - 10)),
                                    cv2.FONT_HERSHEY_SIMPLEX,
                                    1.0, (0, 255, 0), 2)

                        # Scoring logic per detected class_id
                        if int(class_id) == 0:
                            # Speed-limit sign detected
                            penalty = speed_penalty(mqtt_data['v1'])
                            score -= penalty

                        elif int(class_id) == 1:
                            # Example: “school zone” sign
                            current_flag = 1

                        elif int(class_id) == 2:
                            # Example: “construction zone” sign
                            current_flag = 2

                        elif int(class_id) == 3:
                            # Example: “stop sign”
                            current_flag = 3
                            time.sleep(10)  # Pause for 10 seconds to simulate full stop
                            if not check_required_flags():
                                score -= 5

                        elif int(class_id) == 4:
                            # Example: “yield sign”
                            current_flag = 4

                        elif int(class_id) == 5:
                            # Example: “left-turn only” sign
                            # Requires v8 == 4 and v2 == 1
                            if not (mqtt_data['v8'] == 4 and mqtt_data['v2'] == 1):
                                score -= 0.5
                            current_flag = 5

                        elif int(class_id) == 6:
                            # Example: “right-turn only” sign
                            # Requires v8 == 3 and v3 == 1
                            if not (mqtt_data['v8'] == 3 and mqtt_data['v3'] == 1):
                                score -= 0.5
                            current_flag = 6

                        elif int(class_id) == 7:
                            # Example: “pedestrian crossing” sign
                            current_flag = 7

                # After processing detections, apply any flag-based penalties
                if current_flag == 1:  # school zone: speed must be <= 60
                    sp = mqtt_data['v1'] - 100
                    if 61 <= sp <= 100:
                        score -= 0.2
                    elif 101 <= sp <= 150:
                        score -= 0.5

                elif current_flag == 2:  # construction zone: speed must be <= 40
                    sp = mqtt_data['v1'] - 100
                    if 41 <= sp <= 70:
                        score -= 0.2
                    elif 71 <= sp <= 110:
                        score -= 0.4
                    elif 111 <= sp <= 150:
                        score -= 0.8

                # Reset flag after applying its logic
                current_flag = 0

            # Display the frame with annotations
            cv2.imshow("Frame", frame)

            # Print the current score once per processed frame
            print(f"Score: {score:.1f}")

            # Increment frame counter
            frame_count += 1

            # Exit on 'q' key
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    finally:
        # Clean up
        cap.release()
        cv2.destroyAllWindows()
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
