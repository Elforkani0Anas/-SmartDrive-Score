// ============================================================================
/*the ESP32 Master code that:

Receives “drive” commands (forward, backward, left, right) and a speed value via Blynk.

Reads four IR sensors (for line following).

Toggles two LEDs (e.g., to simulate turn signals).

Packages and sends all data (speed + sensor readings) as CAN frames on a 500 kbps CAN bus.
-------------------------------------------------------*/

//   **IMPORTANT**: Before uploading, replace all placeholder strings below.!!!!!!!!


#define BLYNK_TEMPLATE_ID   "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"

#include <Wire.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <CAN.h>

// Infrared sensor pins
#define IR_SENSOR_RIGHT1 2 
#define IR_SENSOR_LEFT1  5 
#define IR_SENSOR_RIGHT2 4  
#define IR_SENSOR_LEFT2 23  

// CAN bus pins
#define TX_GPIO_NUM   17  
#define RX_GPIO_NUM   16 

// Motor driver pins
#define ENA 25
#define IN1 26
#define IN2 27 
#define IN3 12  
#define IN4 14
#define ENB 13

// Onboard LED pins (e.g. turn signals)
#define LED 33
#define LED1 32

bool forward  = false;
bool backward = false;
bool leftDir  = false;
bool rightDir = false;
int  Speed;
int  value;
int  value1;

// Make sure to replace these with your real SSID/password
const char* ssid = "YOUR_WIFI_SSID";
const char* pass = "YOUR_WIFI_PASSWORD";

// --------------------------------------------------
//                SETUP / LOOP
// --------------------------------------------------
void setup() {
  Serial.begin(9600);
  setBlynkAndCAN();
  setInfraredPins();
}

void loop() {
  Blynk.run();
  smartCarDrive();
  readInfraredSensors();
}

// --------------------------------------------------
//        Infrared Sensor Pin Configuration
// --------------------------------------------------
void setInfraredPins() {
  pinMode(IR_SENSOR_RIGHT1, INPUT);
  pinMode(IR_SENSOR_LEFT1,  INPUT);
  pinMode(IR_SENSOR_RIGHT2, INPUT);
  pinMode(IR_SENSOR_LEFT2,  INPUT);
}

// --------------------------------------------------
//     Blynk & CAN Initialization
// --------------------------------------------------
void setBlynkAndCAN() {
  // Motor & LED pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(LED, OUTPUT); 
  pinMode(LED1, OUTPUT);

  // Start Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);

  // Wait for serial (optional)
  while (!Serial);
  delay(10);

  Serial.println("== CAN Sender (ESP32 Master) ==");

  // Configure CAN bus pins
  CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);

  // Start CAN at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Error: Starting CAN failed!");
    while (1);  // freeze here
  } else {
    Serial.println("CAN Initialized at 500 kbps");
  }
}

// --------------------------------------------------
//            BLYNK CALLBACK FUNCTIONS
// --------------------------------------------------
BLYNK_WRITE(V1) { forward  = param.asInt(); }
BLYNK_WRITE(V4) { backward = param.asInt(); }
BLYNK_WRITE(V3) { leftDir  = param.asInt(); }
BLYNK_WRITE(V2) { rightDir = param.asInt(); }
BLYNK_WRITE(V5) { Speed    = param.asInt(); }
BLYNK_WRITE(V6) { toggleLED(LED, param.asInt());    }
BLYNK_WRITE(V7) { toggleLED(LED1, param.asInt());   }

// --------------------------------------------------
//        Drive Logic & CAN Sender Routine
// --------------------------------------------------
void smartCarDrive() {
  if (forward) {
    carForward();
    Serial.println("Driving forward");
  } 
  else if (backward) {
    carBackward();
    Serial.println("Driving backward");
  } 
  else if (leftDir) {
    carTurnLeft();
    Serial.println("Turning left");
  } 
  else if (rightDir) {
    carTurnRight();
    Serial.println("Turning right");
  } 
  else {
    carStop();
    Serial.println("Car stopped");
  }
  canSender();
}

void carForward() {
  analogWrite(ENA, Speed + 100);
  analogWrite(ENB, Speed + 100);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void carBackward() {
  analogWrite(ENA, Speed + 100);
  analogWrite(ENB, Speed + 100);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void carTurnLeft() {
  analogWrite(ENA, Speed + 100);
  analogWrite(ENB, Speed + 100);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void carTurnRight() {
  analogWrite(ENA, Speed + 100);
  analogWrite(ENB, Speed + 100);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void carStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// --------------------------------------------------
//      Toggle Onboard LEDs (e.g., signals)
// --------------------------------------------------
void toggleLED(int pin, int state) {
  digitalWrite(pin, (state == 1) ? HIGH : LOW);
}

// --------------------------------------------------
//        Read Infrared Sensor Values
// --------------------------------------------------
void readInfraredSensors() {
  int sensorValueRight1 = digitalRead(IR_SENSOR_RIGHT1); 
  int sensorValueLeft1  = digitalRead(IR_SENSOR_LEFT1);   
  int sensorValueRight2 = digitalRead(IR_SENSOR_RIGHT2); 
  int sensorValueLeft2  = digitalRead(IR_SENSOR_LEFT2);  

  Serial.print("IR Right1: "); Serial.println(sensorValueRight1);
  Serial.print("IR Left1:  "); Serial.println(sensorValueLeft1);
  Serial.print("IR Right2: "); Serial.println(sensorValueRight2);
  Serial.print("IR Left2:  "); Serial.println(sensorValueLeft2);
  delay(10);
}

// --------------------------------------------------
//          Send All Data on CAN Bus
// --------------------------------------------------
void canSender() {
  Serial.print("Sending CAN packet... ");

  // Packet ID 0x01: Speed
  CAN.beginPacket(0x01);
  CAN.write(Speed);
  CAN.endPacket();
  Serial.println("Sent Speed");
  delay(10);

  // Packet ID 0x04: (reserved value)
  CAN.beginPacket(0x04);
  CAN.write(value);
  CAN.endPacket();
  Serial.println("Sent value (0x04)");
  delay(10);

  // Packet ID 0x05: IR Right1
  CAN.beginPacket(0x05);
  CAN.write(sensorValueRight1);
  CAN.endPacket();
  Serial.println("Sent IR Right1 (0x05)");
  delay(10);

  // Packet ID 0x06: IR Left1
  CAN.beginPacket(0x06);
  CAN.write(sensorValueLeft1);
  CAN.endPacket();
  Serial.println("Sent IR Left1 (0x06)");
  delay(10);

  // Packet ID 0x07: IR Right2
  CAN.beginPacket(0x07);
  CAN.write(sensorValueRight2);
  CAN.endPacket();
  Serial.println("Sent IR Right2 (0x07)");
  delay(10);

  // Packet ID 0x08: IR Left2
  CAN.beginPacket(0x08);
  CAN.write(sensorValueLeft2);
  CAN.endPacket();
  Serial.println("Sent IR Left2 (0x08)");
  delay(10);

  // Packet ID 0x09: (another reserved value)
  CAN.beginPacket(0x09);
  CAN.write(value1);
  CAN.endPacket();
  Serial.println("Sent value1 (0x09)");
  delay(20);
}
