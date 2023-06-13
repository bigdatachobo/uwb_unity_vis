#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "DW1000Ranging.h"
#include "DW1000.h"
#include "DW1000Device.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define BNO055_SAMPLERATE_DELAY_MS (50)

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

const uint8_t PIN_RST = 27; 
const uint8_t PIN_IRQ = 34; 
const uint8_t PIN_SS = 4;   

const char* ssid = "cat6";
const char* password =  "pi3141592";
const char* mqtt_server = "192.168.34.43"; // raspberry pi 4
const int mqtt_port = 1883;
const char* mqtt_user = "";
const char* mqtt_password = "";
const char* mqtt_topic = "bno";

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_BNO055 bno = Adafruit_BNO055();

float d1; // Distance from tag to { "Left anchor A" : "aabb" }
float d2; // Distance from tag to { "Right anchor B" : "ccdd" }

double q0;
double q1;
double q2;
double q3;

void setup() {
  Serial.begin(9600);
  delay(1000);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); 
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);

  DW1000.enableDebounceClock();
  DW1000.enableLedBlinking();
  DW1000.setGPIOMode(MSGP3, LED_MODE);

  DW1000Ranging.startAsTag("7D:00:22:EA:82:60:3B:9C", DW1000.MODE_SHORTDATA_FAST_ACCURACY, false);

  if (!bno.begin()) {
    Serial.print("No BNO055 detected");
    while (1);
  }
  delay(1000);

  connectToWiFi(ssid, password);

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  
  DW1000Ranging.loop();

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }
}

void newRange() {

  String anchorAddress = String(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  float anchorTagDistance = float(DW1000Ranging.getDistantDevice()->getRange() * 2 / 5);

  Serial.print("anchorAddress: ");
  Serial.println(anchorAddress);
  Serial.print("anchorTagDistance: ");
  Serial.println(String(anchorTagDistance));

  if (anchorAddress == "aabb" && anchorTagDistance > 0) {
    // 앵커 A left
    d1 = anchorTagDistance;
  } else if (anchorAddress == "ccdd" && anchorTagDistance > 0) {
    // 앵커 B lefta
    d2 = anchorTagDistance;
  }

  bno055();

  if (d1 < 0 || d2 < 0) {
    d1 = 0;
    d2 = 0;
  }
}

void bno055() {
  imu::Quaternion quat = bno.getQuat();
  
  q0 = double(quat.w());
  q1 = double(quat.x());
  q2 = double(quat.y());
  q3 = double(quat.z());
  
  DynamicJsonDocument doc(1024);
  doc["q0"] = q0; // number
  doc["q1"] = q1; // number
  doc["q2"] = q2; // number
  doc["q3"] = q3; // number
  doc["leftAnchor"] = d1; // number aabb
  doc["rightAnchor"] = d2; // number ccdd
  
  char jsonBuffer[1024];
  serializeJson(doc, jsonBuffer);
  
  client.publish(mqtt_topic, jsonBuffer);
  
  delay(BNO055_SAMPLERATE_DELAY_MS);
}

void connectToWiFi(const char * ssid, const char * pwd){
  int retries = 0;
  WiFi.begin(ssid, pwd);
  while (WiFi.status() != WL_CONNECTED && retries < 15){
    retries++;
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  if(retries == 15){
    Serial.println("Failed to connect to WiFi");
    return;
  }
  Serial.println("Connected to WiFi");
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void newDevice(DW1000Device* device) {
  Serial.print("ranging init; 1 device added ! -> ");
  Serial.print(" short:");
  Serial.println(device->getShortAddress(), HEX);
}

void inactiveDevice(DW1000Device* device) {
  Serial.print("delete inactive device: ");
  Serial.println(device->getShortAddress(), HEX);
}
