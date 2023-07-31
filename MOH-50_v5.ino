/*
   For board T-Beam,
   cd Nextcloud/PC_files/Development/Arduino/MOH-50/MOH-50_v5/
*/
#define DEBUG //comment it if you don't want to use serial monitor

#include <SPI.h>
#include <ArduinoJson.h>

#include "boards.h"

#include "deepSleep.h"
#include "lora.h"
#include "gps.h"

#define tbeam

//JSON
StaticJsonDocument<255> docInput;
StaticJsonDocument<255> docOutput;
String jsonOutput = "";
String incoming = "";

//Relay
bool relay = false;
#define relay_GPIO 4

#define INTERVAL_MESSAGE 30 //30 seconds  3600000 = 1 hour
RTC_DATA_ATTR unsigned long millisOffset = 0;
RTC_DATA_ATTR unsigned long timer = 0;
RTC_DATA_ATTR unsigned long timer1 = 0;
RTC_DATA_ATTR unsigned long timer2 = 0;
RTC_DATA_ATTR unsigned long timer3 = 0;

//packet counter
int sentCounter = 1;
int receiveCounter = 0;
int pckCounter = 0;
uint32_t receiveID = 0;
const char* pckstate;
char buf[21];

//Mine ID
uint32_t chipID = 0;

unsigned long offsetMillis()
{
  return millis() + millisOffset;
}

void setup() {
  initLoRa();

#ifdef DEBUG
  Serial.begin(115200);

  printWakeUpReason();
  printResetReason();

  Serial.println("===========BATTERY===========");
  bool batteryConnect = PMU->isBatteryConnect();
  Serial.println("BATTERY:%s" + (String)batteryConnect ? "CONNECT" : "DISCONNECT");
  if (batteryConnect) {
    Serial.println("BATTERY VOLTAGE: " + (String)PMU->getBattVoltage());
    Serial.println("BATTERY LEVEL PERCENTAGE: " + (String)PMU->getBatteryPercent());
  } else {
    Serial.println("USB VOLTAGE: " + (String)PMU->getVbusVoltage());
  }
  Serial.println("===========BATTERY===========");
#endif

  getRuntime();
  setCpuFrequencyMhz(80);
  timer3 = offsetMillis();

  for (int i = 0; i < 17; i = i + 8) {
    chipID |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  pinMode(relay_GPIO, OUTPUT);
  digitalWrite(relay_GPIO, true);


}

void loop() {
  jsonOutput = "";
  byte packetCounter = 0;
  if (LoRa.parsePacket()) {
    Serial.println("-------- DEBUG RECEIVED PACKET --------");
    onReceive();
  } else {
    if (offsetMillis() - timer > INTERVAL_MESSAGE * 1000) {
      while (packetCounter < 5) {
        if (LoRa.parsePacket()) {
          Serial.println("-------- DEBUG RECEIVED PACKET --------");
          onReceive();
        }
        timer = offsetMillis();
#ifdef DEBUG
        Serial.print("Sending packet: ");
        Serial.println(sentCounter);
        Serial.println("My ID: " + (String)chipID);
#endif
        jsonOutput = "";
        unsigned long timeout1 = millis();
        gpsRequest();//FOR t-beam
        docOutput["packetNumber"] = packetCounter;
        docOutput["ID"] = (String)chipID;

#if defined(tbeam)
        docOutput["voltage"] = PMU->getBatteryPercent();//PMU->getBattVoltage();//FOR t-beam
#endif

        docOutput["status"] = "OK";
        docOutput["runtime"] = buf;
        docOutput["gps"][0] = latitude; //48.57966877;
        docOutput["gps"][1] = longitude; //38.01243977;
        docOutput["relay"] = relay;
        serializeJson(docOutput, jsonOutput);
        loraSender(jsonOutput);
        packetCounter++;
        LoRa.receive();
      }
    }
  }

  if (offsetMillis() - timer3 > 3 * 1000) {
    timer3 = offsetMillis();
    LoRa.sleep();
    goToSleep();
  }
}



void onReceive() {
  receiveCounter++;
  incoming = "";

  while (LoRa.available()) {
    incoming = LoRa.readString();
  }

#ifdef DEBUG
  Serial.println(incoming);
#endif

  deserializeJson(docInput, incoming);

  receiveID = docInput["ID"];
  pckstate = docInput["state"];
  pckCounter = docInput["pckCounter"];
  relay = docInput["relay"];

  if (receiveID == chipID && relay == true) {
    infiniteBoomState();
  }
}

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

void getRuntime() {
  unsigned long runMillis = offsetMillis();
  unsigned long allSeconds = offsetMillis() / 1000;
  int runHours = allSeconds / 3600;
  int secsRemaining = allSeconds % 3600;
  int runMinutes = secsRemaining / 60;
  int runSeconds = secsRemaining % 60;


  sprintf(buf, "%02d:%02d:%02d", runHours, runMinutes, runSeconds);
  Serial.println(buf);
}
