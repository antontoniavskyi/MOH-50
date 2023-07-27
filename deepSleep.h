#include "rom/rtc.h"
#include <driver/rtc_io.h>
#include <SX126x-Arduino.h>

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  2        /* Time ESP32 will go to sleep (in seconds) */

//Function that prints reset reason of esp32
void printWakeUpReason();
void printResetReason();

//Function that put esp32 in deep sleep mode
void goToSleep();
