#include "deepSleep.h"

const char * resetReason[16] = {"Reset reason can not be determined", "Reset due to power-on event", "Reset by external pin (not applicable for ESP32)", "Software reset via esp_restart", "Software reset due to exception/panic", "Reset (software or hardware) due to interrupt watchdog", "Reset due to task watchdog", "Reset due to other watchdogs", "Reset after exiting deep sleep mode", "Brownout reset (software or hardware)", "Reset over SDIO", "Reset by USB peripheral", "RTC Watch dog Reset CPU", "for APP CPU, reseted by PRO CPU", "Reset when the vdd voltage is not stable", "RTC Watch dog reset digital core and rtc module"};


void printWakeUpReason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

void printResetReason() {
  RESET_REASON cpu0WakeupReason = rtc_get_reset_reason(0);
  RESET_REASON cpu1WakeupReason = rtc_get_reset_reason(1);
  Serial.print("CPU0 reset reason: "); Serial.println(resetReason[cpu0WakeupReason]);
  Serial.print("CPU1 reset reason: "); Serial.println(resetReason[cpu1WakeupReason]);
}

void goToSleep()
  {
  // Start waiting for data package
  Radio.Standby();
  SX126xSetDioIrqParams(IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT,
              IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT,
              IRQ_RADIO_NONE, IRQ_RADIO_NONE);
  // To get maximum power savings we use Radio.SetRxDutyCycle instead of Radio.Rx(0)
  // This function keeps the SX1261/2 chip most of the time in sleep and only wakes up short times
  // to catch incoming data packages
  Radio.SetRxDutyCycle(2 * 1024 * 1000 * 15.625, 10 * 1024 * 1000 * 15.625);

  // Go back to bed
  #ifdef DEBUG
  Serial.println("Start sleeping");
  #endif
    // Make sure the DIO1, RESET and NSS GPIOs are hold on required levels during deep sleep
  rtc_gpio_set_direction((gpio_num_t)RADIO_DIO0_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pulldown_en((gpio_num_t)RADIO_DIO0_PIN);
  // Setup deep sleep with wakeup by external source
  esp_sleep_enable_ext0_wakeup((gpio_num_t)RADIO_DIO0_PIN, RISING);
  // Finally set ESP32 into sleep
  esp_deep_sleep_start();
  }
