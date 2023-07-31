//----------------SSID and password for WiFi----------------
const char* ssid = "Q.Agro";
const char* password = "quadrocopter";

//const char* ssid = "Q.Service";
//const char* password = "quadrocopter";

//const char* ssid = "dragino";
//const char* password = "021CGflobYF120";
//----------------SSID and password for WiFi----------------

//----------------SSID and password for Access Point----------------
const char *ssidAP = "MOH-50";
const char *passwordAP = "minePassword";
//----------------SSID and password for Access Point----------------

//----------------Web server----------------
const char* PARAM_INPUT_1 = "mineID";//Parameters for webservers input
const char* PARAM_INPUT_2 = "state";//Parameters for webservers input
const char* PARAM_MESSAGE = "message";

String connection = "";

String inputMessageMineID;
bool inputMessageState;
bool sendBoomToMine = false;

String mineInfo; //String with datat about mines for webserver
String information;
//----------------Web server----------------

//----------------Stopwatch----------------
uint32_t startTime;
uint32_t elapsedTime;
uint32_t elapsedMinutes;
uint32_t elapsedSeconds;
uint32_t elapsedHours;

String lastPacketTime;
bool lastPacketReceived = false;
//----------------Stopwatch----------------

//----------------System----------------
String jsonOutput;

String response;

String incomingString = "";
byte receivedPackageCounter = 0;
String recivedPacketInfo = "";

bool relay = true;
#if defined (tbeam)
#define relay_GPIO 4
#else if (ttgo)
#define relay_GPIO 2
#endif

//LED controls
bool ledstate = false;
unsigned long timer1 = 0;
unsigned long timer2 = 0;
unsigned long timer3 = 0;

//GPS variable for converting
static char varForLatitude[15];
static char varForLongitude[15];

//Reset reason varuiable
const char * resetReason[16] = {"Reset reason can not be determined", "Reset due to power-on event", "Reset by external pin (not applicable for ESP32)", "Software reset via esp_restart", "Software reset due to exception/panic", "Reset (software or hardware) due to interrupt watchdog", "Reset due to task watchdog", "Reset due to other watchdogs", "Reset after exiting deep sleep mode", "Brownout reset (software or hardware)", "Reset over SDIO", "Reset by USB peripheral", "RTC Watch dog Reset CPU", "for APP CPU, reseted by PRO CPU", "Reset when the vdd voltage is not stable", "RTC Watch dog reset digital core and rtc module"};
uint32_t resetCounter;
//----------------System----------------

//----------------LoRa----------------
int rssi;
float snr;
//----------------LoRa----------------

//----------------Structure definition----------------
const int mineCount = 3;
bool savedData;

struct mineStruct {
  String structId;
  String structStatus;
  uint16_t structVoltage;
  float structLatitude;
  float structLongitude;
  char structRuntime[21];

  String structLastRecived;
  int structRssi;
  float structSnr;
};

struct mineStruct mineqwerty[mineCount];
//----------------Structure definition----------------



//----------------Buffer variables----------------
bool receivedPackageIndex;

int packageBuffNumb;
int received_package_counter;
float latitudeBuff;
float longitudeBuff;
bool relayBuff;
//----------------Buffer variables----------------



//----------------JSON Config----------------
StaticJsonDocument<255> docInput;
StaticJsonDocument<255> docOutput;
StaticJsonDocument<255> docMineArray;

String webServerBuffer;
//----------------JSON Config----------------
