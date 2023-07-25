#include "rom/rtc.h"
#include <driver/rtc_io.h>
#include <SX126x-Arduino.h>

//Function that prints reset reason of esp32
void printWakeUpReason();
void printResetReason();

//Function that put esp32 in deep sleep mode
void goToSleep();
