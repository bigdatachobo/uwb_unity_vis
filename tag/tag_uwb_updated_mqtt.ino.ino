#include <SPI.h>
#include "DW1000Ranging.h"
#include "DW1000.h"
#include "WiFi.h"
#include <PubSubClient.h>

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4
 
const uint8_t PIN_RST = 27; 
const uint8_t PIN_IRQ = 34; 
const uint8_t PIN_SS = 4;   

const char * ssid = "ASUS_A1_2.4GHz";
const char * password = "six_dragon_flying_666";

const char* mqttServer = "192.168.50.233";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(1000);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); 
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);

  DW1000.enableDebounceClock();
  DW1000.enableLedBlinking();
  DW1000.setGPIOMode(MSGP3, LED_MODE);

  DW1000Ranging.startAsTag("7D:00:22:EA:82:60:3B:9C", DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);

  connectToWiFi(ssid, password);
  client.setServer(mqttServer, mqttPort);
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
  Serial.print("from: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  Serial.print("\t Range: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(" m");
  Serial.print("\t RX power: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getRXPower());
  Serial.print(" dBm ");
    
  float projectedRange = DW1000Ranging.getDistantDevice()->getRange() * 2 / 5;
  String strData = String(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  strData += ", ";
  strData += String(projectedRange);
  Serial.println(strData);

  if(client.connected()){
    client.publish("uwb/range", strData.c_str());
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
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
