/*
 * ESP32 Safety Bracelet - Utility Functions Implementation
 */

#include "utils.h"
#include "wifi_manager.h"
#include <time.h>

// Global serial output flag
static bool serialOutputEnabled = true;

// Format and print a log message
static void logMessage(LogLevel level, const String& tag, const String& message) {
  if (!serialOutputEnabled) return;
  
  String levelStr;
  switch (level) {
    case LOG_DEBUG:   levelStr = "DEBUG"; break;
    case LOG_INFO:    levelStr = "INFO"; break;
    case LOG_WARNING: levelStr = "WARNING"; break;
    case LOG_ERROR:   levelStr = "ERROR"; break;
    default:          levelStr = "UNKNOWN";
  }
  
  String timestamp = getCurrentTimeString();
  String logLine = "[" + timestamp + "] [" + levelStr + "] [" + tag + "] " + message;
  
  Serial.println(logLine);
}

// Log a debug message
void logDebug(const String& tag, const String& message) {
  logMessage(LOG_DEBUG, tag, message);
}

// Log an info message
void logInfo(const String& tag, const String& message) {
  logMessage(LOG_INFO, tag, message);
}

// Log a warning message
void logWarning(const String& tag, const String& message) {
  logMessage(LOG_WARNING, tag, message);
}

// Log an error message
void logError(const String& tag, const String& message) {
  logMessage(LOG_ERROR, tag, message);
}

// isNetworkConnected moved to wifi_manager.cpp to prevent duplicate definitions

// Get current time as a formatted string
// Format: YYYY-MM-DD HH:MM:SS
String getCurrentTimeString() {
  struct tm timeinfo;
  char buffer[24];
  
  if(!getLocalTime(&timeinfo)) {
    // If time is not set, return a placeholder
    return String("Time not set");
  }
  
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

// feedWatchdog moved to power.cpp to prevent duplicate definitions
