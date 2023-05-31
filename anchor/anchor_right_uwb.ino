/////////////////////////////////////////////////////////////////
/*
  ESP32 + UWB | Indoor Positioning + Unity Visualization
  For More Information: https://youtu.be/c8Pn7lS5Ppg
  Created by Eric N. (ThatProject)
*/
/////////////////////////////////////////////////////////////////

#include <SPI.h>
#include "DW1000Ranging.h"
#include "DW1000.h"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4

// connection pins
//const uint8_t PIN_SCK = 18;
//const uint8_t PIN_MOSI = 23;
//const uint8_t PIN_MISO = 19;

// original pin num
//const uint8_t PIN_SS = 15;
//const uint8_t PIN_RST = 2;
//const uint8_t PIN_IRQ = 22;

// dwm1000 esp32 integrated board pin num
const uint8_t PIN_RST = 27; // reset pin
const uint8_t PIN_IRQ = 34; // irq pin
const uint8_t PIN_SS = 4;   // spi select pin

void setup() {
  Serial.begin(115200);
  delay(1000);
  //init the configuration
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);  //Reset, CS, IRQ pin
  //define the sketch as anchor. It will be great to dynamically change the type of module
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachBlinkDevice(newBlink);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);
  //Enable the filter to smooth the distance
  //DW1000Ranging.useRangeFilter(true);

  DW1000.enableDebounceClock();
  DW1000.enableLedBlinking();
  DW1000.setGPIOMode(MSGP3, LED_MODE);

  // COM5
  //  Left Anchor - short name of this anchor: aabb
//  DW1000Ranging.startAsAnchor("BB:AA:5B:D5:A9:9A:E2:9C", DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);

  // COM6
  //Right Anchor - short name of this anchor: ccdd
  DW1000Ranging.startAsAnchor("DD:CC:5B:D5:A9:9A:E2:9C", DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
}

void loop() {
  DW1000Ranging.loop();
}

void newRange() {
  updateRange(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print("from: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  Serial.print("\t Range: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getRange());
  Serial.print(" m");
  Serial.print("\t RX power: ");
  Serial.print(DW1000Ranging.getDistantDevice()->getRXPower());
  Serial.println(" dBm");
}

void newBlink(DW1000Device* device) {
}

void inactiveDevice(DW1000Device* device) {
  Serial.print("delete inactive device: ");
  Serial.println(device->getShortAddress(), HEX);
}

void updateRange(float range) {
}
