/*
   For board T-Beam,
*/
#define DEBUG

#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

#include <TinyGPS++.h>
#include "boards.h"

#include "deepSleep.h"

#define tbeam

// The TinyGPS++ object
TinyGPSPlus gps;

#define BAND 915E6

//JSON
StaticJsonDocument<255> docInput;
StaticJsonDocument<255> docOutput;
String jsonOutput = "";
String incoming = "";

//Relay
bool relay = false;
#define relay_GPIO 4

float latitude;
float longitude;

#define INTERVAL_MESSAGE 30000 //30 seconds  3600000 = 1 hour
unsigned long timer = 0;
unsigned long timer1 = 0;
unsigned long timer2 = 0;

//packet counter
int sentCounter = 1;
int receiveCounter = 0;
int pckCounter = 0;
uint32_t receiveID = 0;
const char* pckstate;

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//Mine ID
uint32_t chipID = 0;

void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println("DEBUG 1");
#endif

#ifdef DEBUG
Serial.println("DEBUG 2");
  printWakeUpReason();
  printResetReason();
#endif



#if defined(tbeam)
  initBoard();
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);//FOR t-beam
  Serial.println("===========BATTERY===========");
  bool batteryConnect = PMU->isBatteryConnect();
  Serial.println("BATTERY:%s" + (String)batteryConnect ? "CONNECT" : "DISCONNECT");
  if (batteryConnect) {
    Serial.println("BATTERY VOLTAGE: " + (String)PMU->getBattVoltage());
    Serial.println("BATTERY LEVEL PERCENTAGE: " + (String)PMU->getBatteryPercent());
    //Serial.println("CURRENT: " + (String)PMU->getBattDischargeCurrent());
  } else {
    Serial.println("USB VOLTAGE: " + (String)PMU->getVbusVoltage());
    //Serial.println("CURRENT: " + (String)PMU->getVbusCurrent());
  }
  Serial.println("===========BATTERY===========");
#elif defined(ttgo)
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
#endif

  for (int i = 0; i < 17; i = i + 8) {
    chipID |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  pinMode(relay_GPIO, OUTPUT);
  digitalWrite(relay_GPIO, true);

  if (!LoRa.begin(LoRa_frequency)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSyncWord(0xA3);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(62.5E3);
  LoRa.setCodingRate4(4 / 8);
  LoRa.setTxPower(20);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  jsonOutput = "";
  byte packetCounter = 0;


  if (LoRa.parsePacket()) {
    onReceive();
  } else {
    if (millis() - timer > INTERVAL_MESSAGE) {
      Serial.print("Sending packet: ");
      Serial.println(sentCounter);
      Serial.println("My ID: " + (String)chipID);
      jsonOutput = "";
      timer = millis();
      unsigned long timeout1 = millis();
      gpsRequest();//FOR t-beam
      docOutput["packetNumber"] = sentCounter;
      docOutput["ID"] = (String)chipID;

#if defined(tbeam)
      docOutput["voltage"] = PMU->getBattVoltage();//FOR t-beam
#endif
      docOutput["status"] = "OK";

      //JsonArray gps = docOutput.createNestedArray("gps");//FOR t-beam
      //gps.add(latitude);
      //gps.add(longitude);
      docOutput["gps"][0] = 48.57966877;//latitude;
      docOutput["gps"][1] = 38.01243977;//longitude;
      docOutput["relay"] = relay;
      serializeJson(docOutput, jsonOutput);
      while (packetCounter < 5) {
        if (millis() - timeout1 > 300) {
          timeout1 = millis();
          loraSender(jsonOutput);
          packetCounter++;
        }
      }
      sentCounter++;
    }
  }
}

void loraSender(String jsonsender) {
  Serial.println(jsonsender);
  LoRa.beginPacket();
  LoRa.print(jsonsender);
  LoRa.endPacket();
  LoRa.receive();
}

void onReceive() {
  //if (packetSize == 0) return;

  receiveCounter++;
  incoming = "";

  while (LoRa.available()) {
    incoming = LoRa.readString();
  }

  Serial.println(incoming);

  deserializeJson(docInput, incoming);

  receiveID = docInput["ID"];
  pckstate = docInput["state"];
  pckCounter = docInput["pckCounter"];
  relay = docInput["relay"];

  if (receiveID == chipID && relay == true) {
    infiniteBoomState();
  }
  //serializeAndSendAck(pckCounter);
}

#if defined(tbeam)
void gpsRequest() {
  latitude = 0;
  longitude = 0;
  while (Serial1.available() > 0)
    if (gps.encode(Serial1.read())) {
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        //Serial.println(latitude);
        longitude = gps.location.lng();
        //Serial.println(longitude);
      }
    }
  //Serial.println("GPS" + " " +  (String)latitude + " " + (String)longitude);

  Serial.println("GPS STATUS START");
  Serial.print("Latitude: ");
  Serial.println(latitude, 6);
  Serial.print("Longtitude: ");
  Serial.println(longitude, 6);
  Serial.println("GPS STATUS END");
}
#else
void gpsRequest() {}
#endif

void infiniteBoomState() {
  bool ledstate = false;
  while (1) {
    if (millis() - timer1 > 3000) {
      timer1 = millis();
      Serial.println("BOOOOOOOm");
    }
    if (millis() - timer2 > 500) {
      timer2 = millis();
      if (ledstate == false) {
        ledstate = true;
      } else {
        ledstate = false;
      }
      digitalWrite(relay_GPIO, ledstate);
    }
  }
}



/*void serializeAndSendAck(byte ackCounter) {
  jsonOutput = "";
  docOutput["ack"] = ackCounter;
  serializeJson(docOutput, jsonOutput);
  loraSender(jsonOutput);
  }*/
