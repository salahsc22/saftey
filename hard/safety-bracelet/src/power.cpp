#include "power.h"
#include "utils.h"
#include "emergency.h"
#include <ArduinoOTA.h>

// Activity tracking
static unsigned long lastActivityTime = 0;
static bool otaInitialized = false;

// Initialize power management
void powerInit() {
  logInfo("POWER", "Initializing power management");
  
  // Initialize watchdog
  initWatchdog();
  
  // Configure wake-up sources for light sleep
  esp_sleep_enable_timer_wakeup(30 * 1000000);  // Wake after 30 seconds
  esp_sleep_enable_touchpad_wakeup();  // Enable touch wake-up
  
  // Setup touchpad wake sources
  touchAttachInterrupt(SOS_TOUCH_PIN, NULL, TOUCH_THRESHOLD);
  touchAttachInterrupt(BLE_TOGGLE_TOUCH_PIN, NULL, TOUCH_THRESHOLD);
  
  // MPU6050 interrupt setup would be here if hardware supports it
  // This would require connecting MPU interrupt pin to ESP32 and configuring it
  
  // Reset activity timer
  lastActivityTime = millis();
  
  logInfo("POWER", "Power management initialized");
}

// Initialize watchdog timer
void initWatchdog() {
  logInfo("POWER", "Initializing watchdog timer with timeout of " + String(WDT_TIMEOUT) + "s");
  
  // Initialize ESP32 watchdog timer
  esp_task_wdt_init(WDT_TIMEOUT, true);  // Enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);  // Add current thread to WDT watch
}

// Feed watchdog timer to prevent reset
void feedWatchdog() {
  esp_task_wdt_reset();
}

// Enter light sleep mode to conserve power
void enterLightSleep() {
  // Make sure watchdog is fed before sleep
  feedWatchdog();
  
  logInfo("POWER", "Entering light sleep mode");
  
  // Prepare display for sleep
  // TODO: Consider turning off display or showing a sleep message
  
  // Make sure all pending operations are complete
  yield();
  delay(10);
  
  // Enter light sleep
  esp_light_sleep_start();
  
  // Code continues here after waking up
  logInfo("POWER", "Woke up from light sleep");
  
  // Reset activity timer
  lastActivityTime = millis();
  
  // Feed watchdog after waking up
  feedWatchdog();
  
  // Update wake-up reason for debugging
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  // Declare variables outside the switch statement to avoid cross-initialization errors
  touch_pad_t touchpad;
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      logInfo("POWER", "Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      logInfo("POWER", "Wakeup caused by touchpad");
      // Identify which touch pad woke the device
      touchpad = esp_sleep_get_touchpad_wakeup_status();
      logInfo("POWER", "Touch pad " + String((int)touchpad) + " triggered wakeup");
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      logInfo("POWER", "Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      logInfo("POWER", "Wakeup caused by external signal using RTC_CNTL");
      break;
    default:
      logInfo("POWER", "Wakeup was not caused by light sleep: " + String(wakeup_reason));
      break;
  }
}

// Update activity timestamp
void updateActivity() {
  // Check for any activity that should reset the timer
  // This could include button presses, sensor readings, etc.
  
  // For now, just reset the timer periodically in the main loop
  lastActivityTime = millis();
}

// Check if device should enter sleep mode
bool shouldEnterSleep() {
  // Don't sleep if in emergency mode
  if (isInEmergencyMode()) {
    return false;
  }
  
  // Check if it's been long enough since last activity
  return (millis() - lastActivityTime) > SLEEP_AFTER_INACTIVITY;
}

// Setup OTA updates
void setupOTA() {
  if (otaInitialized) {
    return;
  }
  
  logInfo("POWER", "Setting up OTA updates");
  
  // Set up OTA updates
  ArduinoOTA.setHostname("ESP32-SafetyBracelet");
  
  // Set password
  ArduinoOTA.setPassword("safety123");  // Change this to a secure password
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    
    // Pause all normal operations during update
    logInfo("OTA", "Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    logInfo("OTA", "Update complete");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int percentage = (progress / (total / 100));
    logInfo("OTA", "Progress: " + String(percentage) + "%");
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    String errorMsg = "Error: ";
    if (error == OTA_AUTH_ERROR) {
      errorMsg += "Auth Failed";
    } else if (error == OTA_BEGIN_ERROR) {
      errorMsg += "Begin Failed";
    } else if (error == OTA_CONNECT_ERROR) {
      errorMsg += "Connect Failed";
    } else if (error == OTA_RECEIVE_ERROR) {
      errorMsg += "Receive Failed";
    } else if (error == OTA_END_ERROR) {
      errorMsg += "End Failed";
    }
    logError("OTA", errorMsg);
  });
  
  ArduinoOTA.begin();
  otaInitialized = true;
  
  logInfo("POWER", "OTA updates initialized");
}

// Handle OTA updates
void handleOTA() {
  if (otaInitialized) {
    ArduinoOTA.handle();
  }
}
