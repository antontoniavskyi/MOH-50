#include <SPI.h>
#include <LoRa.h>
#include <Console.h>
#include <BridgeClient.h>
#include <Ethernet.h>

#include <ArduinoJson.h>
StaticJsonDocument<255> docInput;
StaticJsonDocument<255> docOutput;
String jsonOutput;


String response;
char buff[300];
// Set center frequency
uint32_t freq = 915E6;

String incomingString = "";

int pckBuff = 0;
double latitudeBuff;
double longitudeBuff;
bool relayBuff;
byte receivedPackageCounter = 0;
String recivedPacketInfo = "";

BridgeClient client;

bool relay = true;

void setup() {
  Bridge.begin(115200);
  Console.begin();
  Serial.begin(115200);
  while (!Serial) {}

  Console.println("LoRa Receiver");

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
  }
  LoRa.receive();
}

void onReceive() {
  //if (packetSize == 0) return;
  incomingString = "";
  bool receivedPackageIndex;

  while (LoRa.available()) {
    incomingstring = LoRa.readString();
  }

  deserializeJson(docInput, incomingstring);

  int packetNumb = docInput["packetNumber"];
  double latitude = doc["gps"][0];
  double longitude = doc["gps"][1];
  bool relay = docInput["relay"];

  /*Рахуємо унікальні пакети. У разі, якщо отримуємо кілька пакетів з однаковою інформацією,
   ми їх зберігаємо у буфері, а виводимо коли отримуємо новий пакет даних, 
   таким чином кількість отриманих унікальних пакетів буде актуальною
  */
  if (packetNumb == pckbuff) {
    receivedPackageIndex = true;
    receivedPackageCounter++;
    latitudeBuff = latitude;
    longitudeBuff = longitude;
    relayBuff = relay;
    recivedPacketInfo = "Received " + (String)receivePckCounter + " packets with RSSI " + (String)LoRa.packetRssi() + " and SNR " + (String)LoRa.packetSnr();
  } else if (packetNumb > pckbuff) {
    pckbuff = packetNumb;
    receivePckCounter = 0;
    if (receivedpackageindex) {
      Console.println(recivedPacketInfo);
      Console.print("Latitude: ");
      Console.println(latitudeBuff);
      Console.print("Longitude: ");
      Console.println(longitudeBuff);
      Console.print("Relay: ");
      Console.println(relaybuff);
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
    sendAck(pckbuff);
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
