#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

// Connection mode enum
enum ConnectionMode {
  WIFI_MODE,
  GPRS_MODE,
  NO_CONNECTION
};

// WiFi/GPRS constants
#define WIFI_AP_NAME "ESP32_Safety_Bracelet"
#define WIFI_AP_PASSWORD "12345678"
#define CONFIG_PORTAL_TIMEOUT 180
#define WIFI_RECONNECT_INTERVAL 30000
#define MAX_RECONNECT_ATTEMPTS 3

// Public functions
void wifiManagerInit();
void wifiManagerProcess();
bool isWiFiConnected();
ConnectionMode getConnectionMode();

// Functions
void wifiInit();
void initializeSimModule();
void checkSimCardStatus();
void checkSignalQuality();
bool connectWifiWithTimeout(int timeoutMs);
void resetWiFiSettings();
void checkConnection();
ConnectionMode getCurrentConnectionMode();

// SMS and call wrapper functions
bool sendSMSToNumber(const char* phoneNumber, const char* message);
bool makeCallToNumber(const char* phoneNumber, int callDurationMs);

bool isNetworkConnected();
bool connectToGPRS();
bool isSimModuleReady();

// Make feedWatchdog accessible to network modules
extern void feedWatchdog();

#endif // WIFI_MANAGER_H
