/*
   curl -X POST 192.168.4.1/post -H "Content-type: application/x-www-form-urlencoded" -d "mineID=8947652&state=1"
*/
#define tbeam
#define AP //change Access Point or Station mode
#define DEBUG //comment it if you don't want to use serial monitor

#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "boards.h"
#include "variables.h"
#include "structure.h"
#include "server.h"

#ifdef DEBUG
#include "logger.h"
#endif


#include <Preferences.h>
Preferences preferences;

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define BAND 915E6

void startLora() {
#if defined(ttgo)
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
#else if (tbeam)
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);//For t-beam
#endif
  while (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    appendFile(SPIFFS, "/log", "Starting LoRa failed!\r\n");
  }
  LoRa.setTxPower(20);
  LoRa.setSyncWord(0xA3);
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(62.5E3);
  LoRa.setCodingRate4(4 / 5);
  LoRa.receive();
  Serial.println("LoRa Initializing OK!");
  appendFile(SPIFFS, "/log", "LoRa Initializing OK!\r\n");
}

void setup() {
  Serial.begin(115200);
  preferences.begin("mine", false);

  received_package_counter = preferences.getUInt("packageCounter", 0);

#ifdef DEBUG
  resetCounter = preferences.getUInt("resetCounter", 0);
  resetCounter++;
  preferences.putUInt("resetCounter", resetCounter);
#endif



#ifdef DEBUG
  initSPIFFS();
  getResetReason();
  getResetCounter();
#endif

  startLora();
  eraseStructure(true, "0");

  pinMode(relay_GPIO, OUTPUT);
  //digitalWrite(relay_GPIO, HIGH);

#if defined(AP)
  IPAddress apIP(192, 168, 23, 1);
  IPAddress apGateway(192, 168, 23, 1);
  IPAddress apMask(255, 255, 255, 0);

  char apConfig[50] = "Setting soft-AP configuration ... ";
  //apConfig += WiFi.softAPConfig(apIP, apGateway, apMask) ? "Ready" : "Failed!";
  strcat(apConfig, WiFi.softAPConfig(apIP, apGateway, apMask) ? "Ready" : "Failed!");
  Serial.println(apConfig);
  strcat(apConfig, "\r\n");

  char apSetup[40] = "Setting soft-AP ... ";
  //apSetup += WiFi.softAP(ssidAP, passwordAP) ? "Ready" : "Failed!";
  strcat(apSetup, WiFi.softAP(ssidAP, passwordAP) ? "Ready" : "Failed!");
  Serial.println(apSetup);
  strcat(apSetup, "\r\n");

#ifdef DEBUG
  appendFile(SPIFFS, "/log", apConfig);
  appendFile(SPIFFS, "/log", apSetup);
#endif

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
#else
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    if (DEBUG) appendFile(SPIFFS, "/log", "WiFi Initializing FAILED!\r\n");
    WiFi.reconnect();
  }

  char ipAddr[30] = "IP Address: ";
  strcat(ipAddr, WiFi.localIP());
  Serial.println(ipAddr);
  strcat(ipAddr, "\r\n");
#ifdef DEBUG
  appendFile(SPIFFS, "/log", ipAddr);
#endif
#endif

  webServerFunctions();
  server.on("/log", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/log");
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  if (LoRa.parsePacket()) {
    onReceive();
    serializeJsonWithMineInfo();
    webServerInformation();
  } else {
    readSerial();
  }

  if (sendBoomToMine) {
    sendBoomToMine = false;
    eraseStructure(false, inputMessageMineID); //false is erase specific cell of structure
    webServerInformation();
    serializeJsonWithMineInfo();
    serializeJsonSendToLora(inputMessageMineID, inputMessageState);
  }


  infiniteLedState();
}

void onReceive() {
  //if (packetSize == 0) return;
  //lastReceivedPacketTime();


  incomingString = "";
  int pckBuff = 0;

  while (LoRa.available()) {
    incomingString = LoRa.readString();
  }

  Serial.println(incomingString);

  DeserializationError error = deserializeJson(docInput, incomingString);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  //deserializeJson(docInput, incomingString);


  int packetNumb = docInput["packetNumber"];
  String mineID = docInput["ID"];
  String mineStatus = docInput["status"];
  uint16_t voltage = docInput["voltage"];
  char runtime[21];
  strcpy(runtime, docInput["runtime"]);
  float latitude = docInput["gps"][0];
  float longitude = docInput["gps"][1];
  bool relay = docInput["relay"];
  rssi = LoRa.packetRssi();
  snr = LoRa.packetSnr();

  /*if(lastPacketReceived == false){
    lastPacketReceived = true;
    lastReceivedPacketTime();
    }*/
  //Serial.println("Received: " + (String)packetNumb + " " + mineID + " " + mineStatus + " " + (String)voltage + " " + (String)latitude + " " + (String)longitude+ " " + (String)rssi + " " + (String)snr);
  saveDataInStructure(mineID, latitude, longitude, voltage, lastPacketTime, mineStatus, rssi, snr, runtime);
  //lastReceivedPacketTimeWebServer();

  /*
    Рахуємо унікальні пакети. У разі, якщо отримуємо кілька пакетів з однаковою інформацією,
    ми їх зберігаємо у буфері, а виводимо коли отримуємо новий пакет даних,
    таким чином кількість отриманих унікальних пакетів буде актуальною
  */
  if (packetNumb == packageBuffNumb) {
    receivedPackageIndex = true;
    receivedPackageCounter++;
    latitudeBuff = latitude;
    longitudeBuff = longitude;
    relayBuff = relay;
    recivedPacketInfo = "Received " + (String)receivedPackageCounter + " packets with RSSI " + (String)LoRa.packetRssi() + " and SNR " + (String)LoRa.packetSnr();
  } else { /* if (packetNumb > packageBuffNumb || packetNumb == 1)*/
    packageBuffNumb = packetNumb;
    receivedPackageCounter = 0;
    if (receivedPackageIndex) {
      Serial.println("************** New Package **************");
      receivedPackageIndex = false;
      Serial.println(recivedPacketInfo);
      Serial.print("Latitude: ");
      Serial.println(latitudeBuff, 6);
      Serial.print("Longitude: ");
      Serial.println(longitudeBuff, 6);
      Serial.print("Relay: ");
      Serial.println(relayBuff);
      received_package_counter++;
      preferences.putUInt("counter", received_package_counter);
      preferences.putUInt("voltage", voltage);
      Serial.print("General package counter: ");
      Serial.println(preferences.getUInt("counter", 0));
      Serial.print("Battery voltage: ");
      Serial.println(preferences.getUInt("voltage", 0));
    } else {
      Serial.println("************** New Package **************");
      receivedPackageCounter++;
      Serial.println("Received 1 packet with RSSI " + (String)LoRa.packetRssi() + " and SNR " + (String)LoRa.packetSnr());
      Serial.print("Latitude: ");
      Serial.println(latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(longitude, 6);
      Serial.print("Relay: ");
      Serial.println(relay);
      received_package_counter++;
      preferences.putUInt("packageCounter", received_package_counter);
      Serial.print("General package counter: ");
      Serial.println(preferences.getUInt("counter", 0));
      Serial.print("Battery voltage: ");
      Serial.println(preferences.getUInt("voltage", 0));
    }
    //sendAck(packageBuffNumb);
    lastPacketReceived = false;
    //startTime = millis();
  }

}

void readSerial() {
  String SerialInput;
  String relayInput;
  while (Serial.available()) {
    SerialInput = Serial.readString();
    SerialInput.trim();
    Serial.println(SerialInput);
    if (SerialInput == "relay ON") relay = true;
    if (SerialInput == "relay OFF") relay = false;
    if (SerialInput == "erasestruct") eraseStructure(true, "0"); //true is to erase all data in structure
    if (SerialInput == "show") showstructure();
    if (SerialInput == "erasecounter") {
      preferences.putUInt("counter", 0);
      received_package_counter = 0;
    }
    Serial.println("Sending packet to ttgo");
    serializeJsonSendToLora(mineqwerty[0].structId, relay);
  }
}

void serializeJsonSendToLora(String boomMineID, bool boomState) {
  unsigned long timeout1 = millis();
  byte packetCounter = 0;
  jsonOutput = "";
  docOutput["ID"] = boomMineID;
  docOutput["relay"] = boomState;
  serializeJson(docOutput, jsonOutput);
  while (packetCounter < 15) {
    if (millis() - timeout1 > 400) {
      Serial.println(jsonOutput);
      sendPacket(jsonOutput);
      packetCounter++;
    }
  }
  LoRa.end();
  startLora();
}

/*void sendAck(int sendpcknumb) {
  jsonOutput = "";
  byte packetCounter1 = 0;
  docOutput["state"] = "ACK";
  docOutput["pckCounter"] = sendpcknumb;
  serializeJson(docOutput, jsonOutput);
  unsigned long timeout2 = millis();

  while (packetCounter1 < 5) {
  if (millis() - timeout2 > 300) {
  sendPacket(jsonOutput);
  packetCounter1++;
  }
  }
  Serial.println("Sent ACK to ESP32");
  }*/

void sendPacket(String packetOutput) {
  LoRa.beginPacket();
  LoRa.print(packetOutput);
  LoRa.endPacket();
}

void serializeJsonWithMineInfo() {
  mineInfo = "";
  /*for (byte i = 0; i < 3; i++) {
    if (i == 2) {
    webServerBuffer = "";
    JsonObject docMineArrayObject = docMineArray.createNestedObject();
    docMineArray["ID"] = mineqwerty[i].structId;
    docMineArray["status"] = mineqwerty[i].structStatus;
    docMineArray["gps"][0] = mineqwerty[i].structLatitude;
    docMineArray["gps"][1] = mineqwerty[i].structLongitude;
    serializeJson(docMineArray, webServerBuffer);
    mineInfo += webServerBuffer;
    break;
    }
    webServerBuffer = "";
    docMineArray["ID"] = mineqwerty[i].structId;
    docMineArray["status"] = mineqwerty[i].structStatus;
    docMineArray["gps"][0] = mineqwerty[i].structLatitude;
    docMineArray["gps"][1] = mineqwerty[i].structLongitude;
    serializeJson(docMineArray, webServerBuffer);
    mineInfo += webServerBuffer + ",";
    }*/
  mineInfo = "[";
  for (byte i = 0; i < mineCount; i++) {
    if (i == mineCount - 1) {
      mineInfo += "{\"ID\":\"" + mineqwerty[i].structId + "\",\"status\":\"" + mineqwerty[i].structStatus + "\",\"gps\":[" + String(mineqwerty[i].structLatitude, 6) + "," + String(mineqwerty[i].structLongitude, 6) + "]}";
      break;
    }
    mineInfo += "{\"ID\":\"" + mineqwerty[i].structId + "\",\"status\":\"" + mineqwerty[i].structStatus + "\",\"gps\":[" + String(mineqwerty[i].structLatitude, 6) + "," + String(mineqwerty[i].structLongitude, 6) + "]},";
  }
  mineInfo += "]";

  //writeMineInfoToFile(mineInfo); //For Dragino LG01
}


void webServerInformation() {
  information = "";
  /*information = "[";
    for (byte i = 0; i < 3; i++) {
    if (i == 2) {
    webServerBuffer = "";
    JsonObject docMineArrayObject = docMineArray.createNestedObject();
    docMineArray["ID"] = mineqwerty[i].structId;
    docMineArray["status"] = mineqwerty[i].structStatus;
    docMineArray["voltage"] = mineqwerty[i].structVoltage;
    //JsonArray docMineArrayGps = docMineArray.createNestedArray("gps");
    //docMineArrayGps.add(mineqwerty[i].structLatitude);
    //docMineArrayGps.add(mineqwerty[i].structLongitude);
    docMineArray["gps"][0] = mineqwerty[i].structLatitude;
    docMineArray["gps"][1] = mineqwerty[i].structLongitude;
    docMineArray["RSSI"] = mineqwerty[i].structRssi;
    docMineArray["SNR"] = mineqwerty[i].structSnr;
    serializeJson(docMineArray, webServerBuffer);
    //Serial.println(webServerBuffer);
    information += webServerBuffer;
    break;
    }
    webServerBuffer = "";
    docMineArray["ID"] = mineqwerty[i].structId;
    docMineArray["status"] = mineqwerty[i].structStatus;
    docMineArray["voltage"] = mineqwerty[i].structVoltage;
    docMineArray["gps"][0] = mineqwerty[i].structLatitude;
    docMineArray["gps"][1] = mineqwerty[i].structLongitude;
    docMineArray["RSSI"] = mineqwerty[i].structRssi;
    docMineArray["SNR"] = mineqwerty[i].structSnr;
    serializeJson(docMineArray, webServerBuffer);
    Serial.println(webServerBuffer);
    information += webServerBuffer + ",";
    }
    information += "]";*/


  information = "[";
  for (byte i = 0; i < mineCount; i++) {

    if (i == mineCount - 1) {
      information += "{\"ID\":\"" + mineqwerty[i].structId + "\",\"status\":\"" + mineqwerty[i].structStatus + ",\"voltage\":" + mineqwerty[i].structVoltage + ",\"runtime\":" + mineqwerty[i].structRuntime + "\",\"gps\":[" + String(mineqwerty[i].structLatitude, 6) + "," + String(mineqwerty[i].structLongitude, 6) + "]" + ",\"RSSI\":" + mineqwerty[i].structRssi + ",\"SNR\":" + mineqwerty[i].structSnr + "}";
      break;
    }
    information += "{\"ID\":\"" + mineqwerty[i].structId + "\",\"status\":\"" + mineqwerty[i].structStatus + ",\"voltage\":" + mineqwerty[i].structVoltage + ",\"runtime\":" + mineqwerty[i].structRuntime + "\",\"gps\":[" + String(mineqwerty[i].structLatitude, 6) + "," + String(mineqwerty[i].structLongitude, 6) + "]" + ",\"RSSI\":" + mineqwerty[i].structRssi + ",\"SNR\":" + mineqwerty[i].structSnr + "},";
  }
  information += "]";

  //writeMineInfoToFile(mineInfo); //For Dragino LG01
}

void infiniteLedState() {
  if (millis() - timer2 > 1000) {
    timer2 = millis();
    if (ledstate == false) {
      ledstate = true;
    } else {
      ledstate = false;
    }
    digitalWrite(relay_GPIO, ledstate);
  }
}

//Mine stopwatch
/*void lastReceivedPacketTime() {
  elapsedTime =   millis() - startTime;         // store elapsed time
  elapsedSeconds = (elapsedTime / 1000);       // divide by 1000 to convert to seconds - then cast to an int to print
  elapsedMinutes = (elapsedSeconds / 60);
  elapsedHours = (elapsedMinutes / 60);

  lastPacketTime = (String)elapsedHours + ":" + (String)elapsedMinutes + ":" + (String)elapsedSeconds;

  }

  void lastReceivedPacketTimeWebServer(){
  information = "";
  information = "[";
  for (byte i = 0; i < 10; i++) {
  if (i == 9) {
  information += "{\"ID\":" + mineqwerty[i].structId + ",\"Time\":" + mineqwerty[i].structLastRecived + "]}";
  break;
  }
  information += "{\"ID\":" + mineqwerty[i].structId + ",\"Time\":" + mineqwerty[i].structLastRecived + "]},";
  }
  information += "]";
  }*/


/*void writeMineInfoToFile(String mineInfoToFile) {
  File dataFile = FileSystem.open("/usr/pythonServer/mineInfo", FILE_APPEND);
  if (dataFile) {
  dataFile.print("[");
  dataFile.println(mineInfoToFile);
  dataFile.close();
  Serial.println(mineInfoToFile);
  }else {
  Serial.println("error opening datalog");
  }
  }*/
