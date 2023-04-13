#include <SPI.h>
#include <LoRa.h>
#include <Console.h>
#include <BridgeClient.h>
#include <FileIO.h>

#include <ArduinoJson.h>
StaticJsonDocument<100> docInput;
StaticJsonDocument<100> docOutput;
StaticJsonDocument<100> docMineArray;
String jsonOutput;

String response;
// Set center frequency
uint32_t freq = 915E6;

String incomingString = "";
byte receivedPackageCounter = 0;
String recivedPacketInfo = "";

BridgeClient client;

bool relay = true;

struct mineStruct {
  char structId[20];
  String structStatus;
  float structLatitude;
  float structLongitude;
};

struct mineStruct mineqwerty[10];

void setup() {
  Bridge.begin(115200);
  Console.begin();
  FileSystem.begin();

  while(!Console);
  Console.println("LoRa Receiver");

  //mineqwerty[0] = { strcpy(mineqwerty[0].structId,"an34ju141ol"), "OK", 50.443222, 30.447847 };  //3792142 is mine ID
  strcpy(mineqwerty[0].structId, "an34ju141ol");
  mineqwerty[0].structStatus = "OK";
  mineqwerty[0].structLatitude = 50.443222;
  mineqwerty[0].structLongitude = 30.447847;

  if (!LoRa.begin(freq)) {
    Console.println("Starting LoRa failed!");
    while (1)
      ;
  }

  LoRa.setSyncWord(0xAC4BF56);
  //LoRa.receive();
}

void loop() {
  if (LoRa.parsePacket()) {
    Serial.println("receive");
    onReceive();
  } else {
    readConsole();
    Console.print("2");
    serializeJsonWithMineInfo();
  }
  LoRa.receive();
  Console.print("1");
}

void onReceive() {
  //if (packetSize == 0) return;
  incomingString = "";
  int pckBuff = 0;
  double latitudeBuff;
  double longitudeBuff;
  bool relayBuff;
  byte packageBuffNumb;
  bool receivedPackageIndex;

  while (LoRa.available()) {
    incomingString = LoRa.readString();
  }

  deserializeJson(docInput, incomingString);

  int packetNumb = docInput["packetNumber"];
  double latitude = docInput["gps"][0];
  double longitude = docInput["gps"][1];
  bool relay = docInput["relay"];

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
  } else if (packetNumb > packageBuffNumb) {
    packageBuffNumb = packetNumb;
    receivedPackageCounter = 0;
    if (receivedPackageIndex) {
      Console.println(recivedPacketInfo);
      Console.print("Latitude: ");
      Console.println(latitudeBuff);
      Console.print("Longitude: ");
      Console.println(longitudeBuff);
      Console.print("Relay: ");
      Console.println(relayBuff);
    } else {
      Console.println("Received 1 packet with RSSI " + (String)LoRa.packetRssi() + " and SNR " + (String)LoRa.packetSnr());
      Console.print("Latitude: ");
      Console.println(latitude);
      Console.print("Longitude: ");
      Console.println(longitude);
      Console.print("Relay: ");
      Console.println(relay);
    }
    Console.println(incomingString);
    sendAck(packageBuffNumb);
  }
}

void readConsole() {
  String consoleInput;
  String relayInput;
  while (Console.available()) {
    consoleInput = Console.readString();
    consoleInput.trim();
    Console.println(consoleInput);
    if (consoleInput == "relay ON") relay = true;
    if (consoleInput == "relay OFF") relay = false;
    Console.println("Sending packet to ttgo");
    serializeJsonSendToLora();
  }
}

void serializeJsonSendToLora() {
  jsonOutput = "";
  byte packetCounter = 0;

  unsigned long timeout1 = millis();

  while (packetCounter < 6) {
    if (millis() - timeout1 > 200) {
      docOutput["packageCounter"] = packetCounter;
      docOutput["relay"] = relay;
      serializeJson(docOutput, jsonOutput);
      sendPacket(jsonOutput);
      packetCounter++;
    }
  }
}

void sendAck(int sendpcknumb) {
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
  Console.println("Sent ACK to ESP32");
}

void sendPacket(String packetOutput) {
  LoRa.beginPacket();
  LoRa.print(packetOutput);
  LoRa.endPacket();
}

void serializeJsonWithMineInfo() {
  String mineInfoOutput;
  JsonObject mine1 = docMineArray.createNestedObject();
  mine1["ID"] = mineqwerty[0].structId;
  mine1["status"] = mineqwerty[0].structStatus;
  mine1["gps"][0] = mineqwerty[0].structLatitude;
  mine1["gps"][1] = mineqwerty[0].structLongitude;
  //JsonArray mine1_gps = mine1.createNestedArray("gps");
  //mine1_gps.add(mineqwerty[0].structLatitude);
  //mine1_gps.add(mineqwerty[0].structLongitude);
  serializeJson(docMineArray, mineInfoOutput);
  writeMineInfoToFile(mineInfoOutput);
}

void writeMineInfoToFile(String mineInfo) {
  File dataFile = FileSystem.open("/www/mineServer/mineInfo", FILE_APPEND);
  if (dataFile) {
    dataFile.println(mineInfo);
    dataFile.close();
    Console.println(mineInfo);
  }else {
    Console.println("error opening datalog.csv");
  }
}
