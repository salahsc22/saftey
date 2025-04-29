#ifndef API_H
#define API_H

#include <Arduino.h>
#include <HTTPClient.h>

// API settings
#define HTTP_TIMEOUT 10000
#define HTTP_MAX_RETRIES 3

// API Authentication
#define API_KEY "safety_bracelet_api_key"  // Replace with your actual API key

// API Endpoints base
#define API_BASE_URL "http://16.170.159.206:8000"

// Functions
void apiInit(const String& userId);
bool sendGpsData(float latitude, float longitude);
bool sendBatteryStatus(int percentage);
bool sendNotification(const char* title, const char* message, int priority);
bool fetchChildData(String* childData);
bool sendHttpRequest(String url, String payload, String* response, int maxRetries = HTTP_MAX_RETRIES);

#endif // API_H
