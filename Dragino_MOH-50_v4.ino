#include <SPI.h>
#include <LoRa.h>
#include <Console.h>
#include <BridgeClient.h>
#include <Ethernet.h>

byte mac[] = { 0xA8, 0x40, 0x41, 0x1B, 0xF1, 0x10 };
EthernetServer server(80);

#include <ArduinoJson.h>
StaticJsonDocument<255> docInput;
StaticJsonDocument<255> docOutput;
StaticJsonDocument<255> docMineArray;
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

struct mineStruct{
  String structStatus;
  double structLatitude;
  double structLongitude;
};

const byte sizeOfArray = 10;
int structArrayOfMine[sizeOfArray];

void setup() {
  Bridge.begin(115200);
  Console.begin();
  Serial.begin(115200);
  while (!Serial) {}

  Console.println("LoRa Receiver");

  Ethernet.begin(mac);
  server.begin();

  mineStruct mineqwerty = {"OK", 50.443222, 30.447847}; //3792142 is mine ID
  structArrayOfMine[0] = &mineqwerty;

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
    incomingString = LoRa.readString();
  }

  deserializeJson(docInput, incomingString);

  int packetNumb = docInput["packetNumber"];
  double latitude = docInput["gps"][0];
  double longitude = docInput["gps"][1];
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
    recivedPacketInfo = "Received " + (String)receivedPackageCounter + " packets with RSSI " + (String)LoRa.packetRssi() + " and SNR " + (String)LoRa.packetSnr();
  } else if (packetNumb > pckbuff) {
    pckbuff = packetNumb;
    receivedPackageCounter = 0;
    if (receivedPackageIndex) {
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

void serializeJsonWithMineInfo() {
  JsonObject mine1 = docMineArray.createNestedObject();
  mine1["ID"] = structArrayOfMine[0];
  mine1["status"] = structArrayOfMine[0].structStatus;
  mine1["gps"][0] = structArrayOfMine[0].structLatitude;
  mine1["gps"][2] = structArrayOfMine[0].structLongitude;
}

void mineServer() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Console.println("new client");
    // an HTTP request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Console.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard HTTP response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");         // refresh the page automatically every 5 sec
          serializeJsonPretty(docMineArray, client);
          client.println();
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Console.println("client disconnected");
  }
}