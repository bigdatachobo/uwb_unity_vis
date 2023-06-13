#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include "DW1000Ng.hpp"
#include "DW1000NgUtils.hpp"
#include "DW1000NgRanging.hpp"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define BNO055_SAMPLERATE_DELAY_MS (500)

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

const uint8_t PIN_RST = 27; 
const uint8_t PIN_IRQ = 34; 
const uint8_t PIN_SS = 4;   

// Fill these in with your information.
const char* ssid = "cat6";
const char* password =  "pi3141592";
const char* mqtt_server = "192.168.34.43"; 
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

device_configuration_t DEFAULT_CONFIG = {
    false,
    true,
    true,
    true,
    false,
    SFDMode::STANDARD_SFD,
    Channel::CHANNEL_5,
    DataRate::RATE_850KBPS,
    PulseFrequency::FREQ_16MHZ,
    PreambleLength::LEN_256,
    PreambleCode::CODE_3
};

interrupt_configuration_t DEFAULT_INTERRUPT_CONFIG = {
    true,
    true,
    true,
    false,
    true
};

void setup() {
  Serial.begin(9600);
  delay(1000);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  
  DW1000Ng::initialize(PIN_SS, PIN_IRQ, PIN_RST);
  DW1000Ng::applyConfiguration(DEFAULT_CONFIG);
  DW1000Ng::applyInterruptConfiguration(DEFAULT_INTERRUPT_CONFIG);
  
  DW1000Ng::attachSentHandler(handleSent);
  DW1000Ng::attachReceivedHandler(handleReceived);

  DW1000Ng::setAntennaDelay(16436);

  if (!bno.begin()) {
    Serial.print("No BNO055 detected");
    while (1);
  }
  delay(1000);

  connectToWiFi(ssid, password);

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  DW1000Ng::loop();

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }
}

void newRange() {

  String anchorAddress = String(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  float anchorTagDistance = DW1000Ranging.getDistantDevice()->getRange() * 2 / 5;

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
  
  q0 = quat.w();
  q1 = quat.x();
  q2 = quat.y();
  q3 = quat.z();
  
  DynamicJsonDocument doc(1024);
  doc["q0"] = q0;
  doc["q1"] = q1;
  doc["q2"] = q2;
  doc["q3"] = q3;
  doc["leftAnchor"] = d1; // aabb
  doc["rightAnchor"] = d2; // ccdd
  
  char jsonBuffer[1024];
  serializeJson(doc, jsonBuffer);
  
  client.publish(mqtt_topic, jsonBuffer);
  
  delay(BNO055_SAMPLERATE_DELAY_MS);
}

void handleSent() {
    // 시간 값 수집
    timePollSent = millis();  // 예시로, 현재 시간을 사용합니다.
}

void handleReceived() {
    // 시간 값 수집
    timePollReceived = millis();  // 예시로, 현재 시간을 사용합니다.

    // 다른 시간 값들도 비슷하게 수집할 수 있습니다.

    // 모든 시간 값들이 수집되었다면, computeRangeAsymmetric 함수 호출
    DW1000NgRanging::computeRangeAsymmetric(timePollSent, timePollReceived, timePollAckSent, timePollAckReceived, timeRangeSent, timeRangeReceived);
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
