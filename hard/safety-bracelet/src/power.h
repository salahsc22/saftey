#ifndef POWER_H
#define POWER_H

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <esp_sleep.h>

// Watchdog settings
#define WDT_TIMEOUT 30   // Watchdog timeout in seconds

// Power management settings
#define SLEEP_AFTER_INACTIVITY 300000   // Sleep after 5 minutes of inactivity

// Functions
void powerInit();
void initWatchdog();
void feedWatchdog();
void enterLightSleep();
void updateActivity();
bool shouldEnterSleep();
void setupOTA();
void handleOTA();

#endif // POWER_H
