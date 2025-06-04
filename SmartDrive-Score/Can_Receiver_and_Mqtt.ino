/*-----------------------------------------------------------
ESP32 code Slave that:

Listens on the CAN bus (500 kbps) for frames sent by the ESP32 Master.

Reads each incoming CAN packet (IDs 0x01–0x0A) and publishes its payload to a corresponding MQTT topic.

Maintains a persistent connection to a Wi-Fi network and an MQTT broker.
----------------------------------------------------------------------------*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <CAN.h>

// -----------------------------------------------------------------------------
//                        CAN IDs & MQTT Topics
// -----------------------------------------------------------------------------
#define CAN_ID_VAR1 0x01
#define CAN_ID_VAR2 0x04  
#define CAN_ID_VAR3 0x05 
#define CAN_ID_VAR4 0x06
#define CAN_ID_VAR5 0x07
#define CAN_ID_VAR6 0x08
#define CAN_ID_VAR7 0x09

const char* mqtt_topic1 = "esp32/v1";
const char* mqtt_topic2 = "esp32/v2";
const char* mqtt_topic3 = "esp32/v3";
const char* mqtt_topic4 = "esp32/v4";
const char* mqtt_topic5 = "esp32/v5";
const char* mqtt_topic6 = "esp32/v6";
const char* mqtt_topic7 = "esp32/v7";
const char* mqtt_topic8 = "esp32/v8";

// -----------------------------------------------------------------------------
//                        Wi-Fi & MQTT Configuration
// -----------------------------------------------------------------------------
const char* ssid       = "YOUR_WIFI_SSID";      // replace with your SSID
const char* password   = "YOUR_WIFI_PASSWORD";  // replace with your Wi-Fi password
const char* mqtt_server = "mqtt.eclipseprojects.io";
const int   mqtt_port   = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// -----------------------------------------------------------------------------
//                             CAN Bus Pins
// -----------------------------------------------------------------------------
#define TX_GPIO_NUM 17  // Connects to CTX
#define RX_GPIO_NUM 16  // Connects to CRX

// -----------------------------------------------------------------------------
//                            Helper: Publish Function
// -----------------------------------------------------------------------------
void publishValue(const char* topic, int value) {
  // Convert integer to string and publish once
  char buff[6];              // enough for values 0–65535
  itoa(value, buff, 10);     // integer → ASCII (base 10)
  client.publish(topic, buff);
  Serial.print("Published ");
  Serial.print(buff);
  Serial.print(" to ");
  Serial.println(topic);
}

// -----------------------------------------------------------------------------
//                       Connect to Wi-Fi & MQTT
// -----------------------------------------------------------------------------
void setup_wifi() {
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Wi-Fi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  // Loop until we’re connected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // You can set a unique client ID if needed
    if (client.connect("ESP32_CAN_Receiver")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(". Retrying in 5 seconds");
      delay(5000);
    }
  }
}

// -----------------------------------------------------------------------------
//                             Setup & Loop
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Initialize CAN bus
  CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);
  if (!CAN.begin(500E3)) {
    Serial.println("Error: CAN initialization failed!");
    while (1) {
      delay(1000);
      Serial.println("Retrying CAN...");
      if (CAN.begin(500E3)) {
        Serial.println("CAN re-initialized.");
        break;
      }
    }
  }
  Serial.println("CAN initialized at 500 kbps");
}

void loop() {
  // Ensure MQTT stays connected
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Check for a CAN packet
  int packetSize = CAN.parsePacket();
  if (packetSize) {
    int id    = CAN.packetId();
    int value = CAN.read();

    Serial.print("Received CAN ID: 0x");
    Serial.print(id, HEX);
    Serial.print("  Value: ");
    Serial.println(value);

    // Publish based on ID
    switch (id) {
      case CAN_ID_VAR1:
        publishValue(mqtt_topic1, value);
        break;
      case CAN_ID_VAR2:
        publishValue(mqtt_topic2, value);
        break;
      case CAN_ID_VAR3:
        publishValue(mqtt_topic3, value);
        break;
      case CAN_ID_VAR4:
        publishValue(mqtt_topic4, value);
        break;
      case CAN_ID_VAR5:
        publishValue(mqtt_topic5, value);
        break;
      case CAN_ID_VAR6:
        publishValue(mqtt_topic6, value);
        break;
      case CAN_ID_VAR7:
        publishValue(mqtt_topic7, value);
        break;
      case CAN_ID_VAR8:
        publishValue(mqtt_topic8, value);
        break;
      default:
        Serial.println("Unknown CAN ID; ignoring");
        break;
    }
  }
  // No “delay” here—letting loop run as fast as possible 
  // reduces latency. If you really need a pause, insert a shorter delay.
}
