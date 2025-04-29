#include "api.h"
#include "utils.h"
#include "wifi_manager.h"
#include <ArduinoJson.h>

// API endpoint URLs
static String latitudeApiUrl;
static String longitudeApiUrl;
static String notificationApiUrl;
static String batteryStatusApiUrl;
static String childDataApiUrl;
static String userId;

// Initialize API with user ID
void apiInit(const String& id) {
  // Store user ID
  userId = id;
  logInfo("API", "Initializing API with userId: " + userId);
  
  // Construct API endpoint URLs
  latitudeApiUrl = String(API_BASE_URL) + "/save-latitude/";
  longitudeApiUrl = String(API_BASE_URL) + "/save-longitude/" + userId + "/";
  notificationApiUrl = String(API_BASE_URL) + "/add-notification/" + userId + "/";
  batteryStatusApiUrl = String(API_BASE_URL) + "/save-battery-status/" + userId + "/";
  childDataApiUrl = String(API_BASE_URL) + "/child_data/" + userId + "/";
  
  logInfo("API", "API endpoints configured");
}

// Send GPS data to API
bool sendGpsData(float latitude, float longitude) {
  if (!isNetworkConnected()) {
    logError("API", "Network not connected, cannot send GPS data");
    return false;
  }
  
  logInfo("API", "Sending GPS data: Lat: " + String(latitude, 6) + ", Lon: " + String(longitude, 6));
  
  // Create JSON payloads
  StaticJsonDocument<128> latDoc;
  latDoc["latitude"] = latitude;
  latDoc["userId"] = userId;
  
  StaticJsonDocument<128> lonDoc;
  lonDoc["longitude"] = longitude;
  
  // Serialize JSON
  String latPayload;
  String lonPayload;
  serializeJson(latDoc, latPayload);
  serializeJson(lonDoc, lonPayload);
  
  // Send both latitude and longitude
  bool latSuccess = false;
  bool lonSuccess = false;
  String response;
  
  latSuccess = sendHttpRequest(latitudeApiUrl, latPayload, &response);
  if (latSuccess) {
    logInfo("API", "Latitude sent successfully");
  } else {
    logError("API", "Failed to send latitude");
  }
  
  lonSuccess = sendHttpRequest(longitudeApiUrl, lonPayload, &response);
  if (lonSuccess) {
    logInfo("API", "Longitude sent successfully");
  } else {
    logError("API", "Failed to send longitude");
  }
  
  return latSuccess && lonSuccess;
}

// Send battery status to API
bool sendBatteryStatus(int percentage) {
  if (!isNetworkConnected()) {
    logError("API", "Network not connected, cannot send battery status");
    return false;
  }
  
  logInfo("API", "Sending battery status: " + String(percentage) + "%");
  
  // Create JSON payload
  StaticJsonDocument<128> doc;
  doc["batteryPercentage"] = percentage;
  
  // Serialize JSON
  String payload;
  serializeJson(doc, payload);
  
  // Send request
  String response;
  bool success = sendHttpRequest(batteryStatusApiUrl, payload, &response);
  
  if (success) {
    logInfo("API", "Battery status sent successfully");
  } else {
    logError("API", "Failed to send battery status");
  }
  
  return success;
}

// Send notification to API
bool sendNotification(const char* title, const char* message, int priority) {
  if (!isNetworkConnected()) {
    logError("API", "Network not connected, cannot send notification");
    return false;
  }
  
  logInfo("API", "Sending notification: " + String(title) + " - " + String(message));
  
  // Create JSON payload
  StaticJsonDocument<256> doc;
  doc["title"] = title;
  doc["message"] = message;
  doc["priority"] = priority;
  doc["delivered_at"] = getCurrentTimeString(); // Campo renombrado de timestamp a delivered_at
  
  // Serialize JSON
  String payload;
  serializeJson(doc, payload);
  
  // Send request
  String response;
  bool success = sendHttpRequest(notificationApiUrl, payload, &response);
  
  if (success) {
    logInfo("API", "Notification sent successfully");
  } else {
    logError("API", "Failed to send notification");
  }
  
  return success;
}

// Fetch child data from API
bool fetchChildData(String* childData) {
  if (!isNetworkConnected()) {
    logError("API", "Network not connected, cannot fetch child data");
    return false;
  }
  
  logInfo("API", "Fetching child data from API");
  
  // Send request
  String response;
  bool success = sendHttpRequest(childDataApiUrl, "", &response);
  
  if (success && response.length() > 0) {
    *childData = response;
    logInfo("API", "Child data fetched successfully");
    return true;
  } else {
    logError("API", "Failed to fetch child data");
    return false;
  }
}

// Enhanced HTTP request with proper error handling and response validation
bool sendHttpRequest(String url, String payload, String* response, int maxRetries) {
  bool success = false;
  int attempts = 0;
  
  while (!success && attempts < maxRetries) {
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT);
    
    // Begin connection
    http.begin(url);
    
    // Add headers including authentication
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-KEY", API_KEY);
    
    // Send request
    int httpCode;
    if (payload.length() > 0) {
      httpCode = http.POST(payload);
    } else {
      httpCode = http.GET();
    }
    
    // Check response
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      String responseStr = http.getString();
      
      // If response is not empty, try to parse it as JSON
      if (responseStr.length() > 0) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, responseStr);
        
        if (!error) {
          // Check for success indicator in response
          if (doc.containsKey("success")) {
            success = doc["success"].as<bool>();
          } else {
            // No explicit success field, assume success since status code is 200
            success = true;
          }
        } else {
          // Response is not valid JSON, but status code is 200, so it's probably OK
          logInfo("API", "Response is not JSON but status code is OK: " + responseStr);
          success = true;
        }
      } else {
        // Empty response but status code is 200, consider it a success
        success = true;
      }
      
      // Store response if pointer provided
      if (response != NULL) {
        *response = responseStr;
      }
      
      if (success) {
        logInfo("API", "HTTP request successful: " + url);
      } else {
        logError("API", "API returned error in response body");
      }
    } else {
      logError("API", "HTTP request failed with code: " + String(httpCode));
    }
    
    http.end();
    attempts++;
    
    // If request failed, wait before retry with progressive backoff
    if (!success && attempts < maxRetries) {
      int delayTime = 500 * attempts;
      logInfo("API", "Retrying in " + String(delayTime) + "ms...");
      delay(delayTime);
    }
  }
  
  return success;
}
