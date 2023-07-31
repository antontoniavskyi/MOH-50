#include "lora.h"

void initLoRa() {
#if defined(tbeam)
  initBoard();
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);//FOR t-beam

#elif defined(ttgo)
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
#endif

  while (!LoRa.begin(LoRa_frequency)) {
    Serial.println("Starting LoRa failed!");
  }
  LoRa.setSyncWord(0xA3);
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(62.5E3);
  LoRa.setCodingRate4(4 / 5);
  LoRa.setTxPower(20);
#ifdef DEBUG
  Serial.println("LoRa Initializing OK!");
#endif
}

void loraSender(String jsonsender) {
#ifdef DEBUG
  Serial.println(jsonsender);
#endif
  LoRa.beginPacket();
  LoRa.print(jsonsender);
  LoRa.endPacket();
  LoRa.receive();
}
