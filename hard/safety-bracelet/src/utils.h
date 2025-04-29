/*
 * ESP32 Safety Bracelet - Utility Functions
 * 
 * This file contains utility functions and constants used throughout the application.
 */

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <HTTPClient.h>

// API constants
#define API_BASE_URL "http://16.170.159.206:8000"
#define API_KEY "your-api-key-here" // Replace with actual API key if available
#define HTTP_TIMEOUT 10000 // 10 seconds

// Log levels
enum LogLevel {
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR
};

// Logging functions
void logDebug(const String& tag, const String& message);
void logInfo(const String& tag, const String& message);
void logWarning(const String& tag, const String& message);
void logError(const String& tag, const String& message);

// Network status - defined in wifi_manager.cpp
extern bool isNetworkConnected();

// Time functions
String getCurrentTimeString();

// Watchdog functions - defined in power.cpp
extern void feedWatchdog();

// HTTP status codes
#define HTTP_CODE_OK 200
#define HTTP_CODE_CREATED 201
#define HTTP_CODE_BAD_REQUEST 400
#define HTTP_CODE_UNAUTHORIZED 401
#define HTTP_CODE_NOT_FOUND 404
#define HTTP_CODE_SERVER_ERROR 500

#endif // UTILS_H
