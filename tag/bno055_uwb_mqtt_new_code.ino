#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DW1000Ranging.h"
#include "DW1000.h"
#include "DW1000Device.h"

// FreeRTOS functions
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BNO055_SAMPLERATE_DELAY_MS (50)

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

const uint8_t PIN_RST = 27; 
const uint8_t PIN_IRQ = 34; 
const uint8_t PIN_SS = 4;  

#define WIFI_SSID "cat6"
#define WIFI_PASSWORD "pi3141592"
#define MQTT_SERVER "192.168.34.43"
#define MQTT_PORT 1883

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_BNO055 bno = Adafruit_BNO055();

double q0, q1, q2, q3;
double d1, d2;

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void connectToMqtt() {
  client.setServer(MQTT_SERVER, MQTT_PORT);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void Task1(void *parameter) // BNO055 관련 작업을 수행하는 태스크
{
  for(;;)
  {
    imu::Quaternion quat = bno.getQuat(); // Quaternion 값을 읽습니다.
    q0 = quat.w(), q1 = quat.x(), q2 = quat.y(), q3 = quat.z();
    vTaskDelay(BNO055_SAMPLERATE_DELAY_MS / portTICK_PERIOD_MS);
  }
}

void Task2(void *parameter) // UWB 거리 측정 관련 작업을 수행하는 태스크
{
  for(;;)
  {
    String anchorAddress = String(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
    double anchorTagDistance = double(DW1000Ranging.getDistantDevice()->getRange() * 2 / 5);
  
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
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  
  if(!bno.begin())
  {
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  delay(1000);

  connectToWiFi();
  connectToMqtt();
  
  // Create task
  xTaskCreate(Task1, "Task_1", 10000, NULL, 1, NULL);
  xTaskCreate(Task2, "Task_2", 10000, NULL, 2, NULL);
}

void loop()
{
  if (!client.connected()) {
    connectToMqtt();
  }
  client.loop();

  DynamicJsonDocument doc(1024);
  doc["q0"] = q0;
  doc["q1"] = q1;
  doc["q2"] = q2;
  doc["q3"] = q3;
  doc["leftAnchor"] = d1;
  doc["rightAnchor"] = d2;
    
  char buffer[1024];
  serializeJson(doc, buffer);

  client.publish("uwb", buffer);

  delay(500);
}
