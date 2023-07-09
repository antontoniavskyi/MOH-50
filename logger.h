#include "SPIFFS.h"
#include "FS.h"

void readFile(fs::FS &fs, const char * path);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);

//Initialose filesystem for log file, and write header to it
void initSPIFFS() {
  while (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }
  writeFile(SPIFFS, "/log", "=====================================\r\n");
  appendFile(SPIFFS, "/log", "    NEW BOOT AFTER POWERING ON\r\n");
  appendFile(SPIFFS, "/log", "=====================================\r\n");
}

//get reason of last esp32 reset 
void getResetReason() {
  byte r = esp_reset_reason();
  Serial.println(resetReason[r]);
  appendFile(SPIFFS, "/log", resetReason[r]);
  appendFile(SPIFFS, "/log", "\r\n");
}

void getResetCounter(){

  char resetCounterMessageForLogger[30] = "ESP32 have restarted ";
  char resetCounterForLogger[10];
  itoa(resetCounter,resetCounterForLogger,10);
  strcat(resetCounterMessageForLogger, resetCounterForLogger);
  appendFile(SPIFFS, "/log", resetCounterMessageForLogger);
  appendFile(SPIFFS, "/log", "\r\n");
}

void readFile(fs::FS &fs, const char * path) {
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  file.print(message);
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
    file.print(message);
  file.close();
}
