#include "ble_manager.h"
#include "utils.h"
#include "storage.h"

// Private variables
static BLEServer *pServer = NULL;
static BLECharacteristic *pTxCharacteristic = NULL;
static BLECharacteristic *pRxCharacteristic = NULL;
static BLECharacteristic *pAuthCharacteristic = NULL;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;
static bool bleEnabled = true;
static bool bleAuthenticated = false;
static String userId = "";
static String deviceName = "";
static bool userIdReceived = false;
static uint32_t connectionStartTime = 0;
static uint32_t lastActivityTime = 0;
static uint32_t securityToken = 0;
static unsigned long lastHeartbeatTime = 0;
static uint8_t connectionAttempts = 0;
static const uint8_t MAX_CONNECTION_ATTEMPTS = 5;

// BLE server callbacks with enhanced security and logging
class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    connectionStartTime = millis();
    lastActivityTime = millis();
    bleAuthenticated = false; // Reset authentication on new connection
    connectionAttempts++;
    
    // Get connecting device info if available
    // Add the required boolean parameter (true for client devices)
    if (pServer->getPeerDevices(true).size() > 0) {
      // Convert the client address to string directly without using BLEAddress constructor
      String address = String(pServer->getPeerDevices(true).begin()->first);
      logInfo("BLE", "Device connected. MAC: " + address + ", Attempt: " + String(connectionAttempts));
    } else {
      logInfo("BLE", "Device connected. Attempt: " + String(connectionAttempts));
    }
    
    // Implement connection throttling if too many attempts
    if (connectionAttempts > MAX_CONNECTION_ATTEMPTS) {
      logWarning("BLE", "Too many connection attempts detected. Throttling BLE.");
      // Use non-blocking delay with watchdog feed
      unsigned long throttleStart = millis();
      while (millis() - throttleStart < 5000) {
        feedWatchdog();
        delay(100); // Short delay that won't block watchdog
      }
      
      // Auto-disable BLE if suspicious activity detected
      if (connectionAttempts > MAX_CONNECTION_ATTEMPTS + 5) {
        logWarning("BLE", "Suspicious connection pattern detected. Disabling BLE temporarily.");
        enableBLE(false);
        // Auto re-enable after 5 minutes
        saveBool("ble_temp_disabled", true);
        saveULong("ble_reenable_time", millis() + 300000);
      }
    }
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    
    // Log connection duration
    uint32_t connectionDuration = (millis() - connectionStartTime) / 1000; // in seconds
    logInfo("BLE", "Device disconnected. Connection duration: " + String(connectionDuration) + " seconds");
    
    // Reset authentication state
    bleAuthenticated = false;
    
    // Reset connection attempts if we had a successful connection
    if (connectionDuration > 10) { // Only reset if connection lasted more than 10 seconds
      connectionAttempts = 0;
    }
  }
};

// RX Characteristic callbacks (for data received from app)
class RxCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      String data = String(value.c_str());
      lastActivityTime = millis();
      
      // Check auth status before processing commands
      if (!bleAuthenticated && !data.startsWith("AUTH:")) {
        logWarning("BLE", "Unauthorized command attempt: " + data);
        sendResponse("ERROR:UNAUTHORIZED");
        return;
      }
      
      logInfo("BLE", "Received: " + data);
      
      // Handle different command types
      if (data.startsWith("UID:")) {
        // Process userId if authorized
        if (bleAuthenticated) {
          userId = data.substring(4);
          logInfo("BLE", "User ID received: " + userId);
          userIdReceived = true;
          sendResponse("OK:UID");
        }
      }
      else if (data.startsWith("AUTH:")) {
        // Process authentication request
        processAuthRequest(data.substring(5));
      }
      else if (data.startsWith("NAME:")) {
        // Set device name
        deviceName = data.substring(5);
        saveString("device_name", deviceName);
        logInfo("BLE", "Device name set: " + deviceName);
        sendResponse("OK:NAME");
        
        // Update BLE advertising name
        updateDeviceName();
      }
      else if (data.startsWith("PING")) {
        // Simple heartbeat check
        sendResponse("PONG:" + String(millis()));
      }
      else if (data.startsWith("BAT")) {
        // Return battery level
        sendResponse("BAT:" + String(getBatteryPercentage()));
      }
      else if (data.startsWith("RESET")) {
        // Process reset command
        logWarning("BLE", "Reset command received. Resetting device...");
        sendResponse("OK:RESET");
        // Make sure the response has time to send
        unsigned long resetDelay = millis();
        while (millis() - resetDelay < 1000) {
          feedWatchdog();
          delay(50);
        }
        ESP.restart();
      }
      else {
        // Unknown command
        sendResponse("ERROR:UNKNOWN_CMD");
      }
    }
  }
  
  // Process authentication request
  void processAuthRequest(String authData) {
    // Split auth string to get token and check for proper formatting
    int separatorIndex = authData.indexOf(':');
    if (separatorIndex == -1) {
      logWarning("BLE", "Invalid auth format received");
      sendResponse("ERROR:AUTH_FORMAT");
      return;
    }
    
    // Extract token and timestamp
    String receivedToken = authData.substring(0, separatorIndex);
    String timestamp = authData.substring(separatorIndex + 1);
    
    // Load passkey from storage (default to a simple key if not set)
    String savedPasskey = loadString("ble_passkey", "safety123");
    
    // Generate expected token
    uint32_t timestampValue = timestamp.toInt();
    String expectedToken = generateAuthToken(savedPasskey, timestampValue);
    
    // Validate token (simple comparison for now)
    if (receivedToken.equals(expectedToken)) {
      bleAuthenticated = true;
      securityToken = random(100000, 999999); // Generate session token
      logInfo("BLE", "Authentication successful");
      sendResponse("AUTH_OK:" + String(securityToken));
    } else {
      logWarning("BLE", "Authentication failed");
      sendResponse("ERROR:AUTH_FAILED");
      
      // Throttle response on auth failure with watchdog feed
      unsigned long throttleStart = millis();
      while (millis() - throttleStart < 1000) {
        feedWatchdog();
        delay(50);
      }
    }
  }
  
  // Generate authentication token from passkey and timestamp
  String generateAuthToken(String passkey, uint32_t timestamp) {
    // Simple hash combining passkey and timestamp
    // In production, use a proper HMAC or similar
    uint32_t hash = 5381;
    for (uint8_t i = 0; i < passkey.length(); i++) {
      hash = ((hash << 5) + hash) + passkey.charAt(i);
    }
    hash = hash ^ timestamp;
    return String(hash, 16); // Hexadecimal representation
  }
  
  // Send response via TX characteristic
  void sendResponse(String response) {
    if (deviceConnected && pTxCharacteristic != NULL) {
      pTxCharacteristic->setValue(response.c_str());
      pTxCharacteristic->notify();
      logInfo("BLE", "Sent: " + response);
    }
  }
};

// Auth Characteristic callbacks (for changing pairing passkey)
class AuthCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0 && bleAuthenticated) {
      String data = String(value.c_str());
      
      // Process passkey change (format: OLD_PASSKEY:NEW_PASSKEY)
      int separatorIndex = data.indexOf(':');
      if (separatorIndex != -1) {
        String oldPasskey = data.substring(0, separatorIndex);
        String newPasskey = data.substring(separatorIndex + 1);
        
        // Verify old passkey
        String savedPasskey = loadString("ble_passkey", "safety123");
        if (oldPasskey.equals(savedPasskey)) {
          // Save new passkey
          if (newPasskey.length() >= 6) {
            saveString("ble_passkey", newPasskey);
            logInfo("BLE", "BLE passkey updated successfully");
            
            // Respond via TX characteristic
            if (pTxCharacteristic != NULL) {
              pTxCharacteristic->setValue("OK:PASSKEY_UPDATED");
              pTxCharacteristic->notify();
            }
          } else {
            logWarning("BLE", "New passkey too short (min 6 chars)");
            if (pTxCharacteristic != NULL) {
              pTxCharacteristic->setValue("ERROR:PASSKEY_TOO_SHORT");
              pTxCharacteristic->notify();
            }
          }
        } else {
          logWarning("BLE", "Passkey change failed: invalid old passkey");
          if (pTxCharacteristic != NULL) {
            pTxCharacteristic->setValue("ERROR:INVALID_OLD_PASSKEY");
            pTxCharacteristic->notify();
          }
        }
      }
    }
  }
};

// Update device name from storage
void updateDeviceName() {
  // Get device name from storage or use default
  deviceName = loadString("device_name", "");
  String advertisingName;
  
  if (deviceName.length() > 0) {
    advertisingName = "SafetyBracelet_" + deviceName;
  } else {
    advertisingName = "ESP32_Safety_Bracelet";
  }
  
  // Update the advertising name if device is running
  if (pServer != NULL) {
    // Need to stop advertising first
    BLEDevice::stopAdvertising();
    
    // Change name
    BLEDevice::deinit(false);
    BLEDevice::init(advertisingName.c_str());
    
    // Restart advertising
    BLEDevice::startAdvertising();
    
    logInfo("BLE", "Updated advertising name to: " + advertisingName);
  }
}

// Initialize BLE
void bleInit() {
  logInfo("BLE", "Initializing BLE");
  
  // Check if BLE was temporarily disabled due to suspicious activity
  if (loadBool("ble_temp_disabled", false)) {
    unsigned long reenableTime = loadULong("ble_reenable_time", 0);
    if (millis() < reenableTime) {
      logInfo("BLE", "BLE temporarily disabled due to security concerns. Will re-enable later.");
      bleEnabled = false;
      return;
    } else {
      // Re-enable BLE
      saveBool("ble_temp_disabled", false);
      logInfo("BLE", "Re-enabling BLE after security timeout");
    }
  }
  
  // Get device name from storage
  deviceName = loadString("device_name", "");
  String advertisingName;
  
  if (deviceName.length() > 0) {
    advertisingName = "SafetyBracelet_" + deviceName;
  } else {
    advertisingName = "ESP32_Safety_Bracelet";
  }
  
  // Create the BLE Device
  BLEDevice::init(advertisingName.c_str());
  
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create BLE Characteristics
  // TX Characteristic (notifications from bracelet to app)
  pTxCharacteristic = pService->createCharacteristic(
                        TX_CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());
  
  // RX Characteristic (commands from app to bracelet)
  pRxCharacteristic = pService->createCharacteristic(
                        RX_CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new RxCharacteristicCallbacks());
  
  // Auth Characteristic (for changing security passkey)
  pAuthCharacteristic = pService->createCharacteristic(
                          AUTH_CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_WRITE
                        );
  pAuthCharacteristic->setCallbacks(new AuthCharacteristicCallbacks());
  
  // Start the service
  pService->start();
  
  // Configure advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  
  // Start advertising
  BLEDevice::startAdvertising();
  
  // Reset connection tracking
  connectionAttempts = 0;
  
  logInfo("BLE", "BLE initialized, advertising as: " + advertisingName);
}

// Handle BLE events
void bleHandleEvents() {
  // Only process if BLE is enabled
  if (!bleEnabled) {
    return;
  }
  
  // Handle disconnection and reconnection
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    
    // Send initial state update when connected
    if (pTxCharacteristic != NULL) {
      String statusMsg = "STATUS:";
      statusMsg += "BAT=" + String(getBatteryPercentage()) + ",";
      statusMsg += "GPS=" + String(isGpsValid() ? "1" : "0") + ",";
      statusMsg += "NET=" + String(isNetworkConnected() ? "1" : "0");
      
      pTxCharacteristic->setValue(statusMsg.c_str());
      pTxCharacteristic->notify();
    }
  }
  
  if (!deviceConnected && oldDeviceConnected) {
    // Give the Bluetooth stack time to get ready with watchdog feed
    unsigned long stackDelay = millis();
    while (millis() - stackDelay < 500) {
      feedWatchdog();
      delay(50);
    }
    if (bleEnabled) {
      pServer->startAdvertising();  // Restart advertising
      logInfo("BLE", "Advertising restarted");
    }
    oldDeviceConnected = deviceConnected;
  }
  
  // Periodic heartbeat when connected (every 30 seconds)
  if (deviceConnected && millis() - lastHeartbeatTime > 30000) {
    if (pTxCharacteristic != NULL) {
      String heartbeatMsg = "HB:";
      heartbeatMsg += String(millis());
      pTxCharacteristic->setValue(heartbeatMsg.c_str());
      pTxCharacteristic->notify();
    }
    lastHeartbeatTime = millis();
  }
  
  // Auto-disconnect if inactive for too long (security feature)
  if (deviceConnected && bleAuthenticated && millis() - lastActivityTime > 300000) { // 5 minutes
    logInfo("BLE", "Auto-disconnecting due to inactivity");
    // Forcing disconnect is not directly supported, but we can restart BLE to achieve this
    BLEDevice::deinit(false);
    // Allow time for deinit to complete with watchdog feed
    unsigned long deinitDelay = millis();
    while (millis() - deinitDelay < 500) {
      feedWatchdog();
      delay(50);
    }
    bleInit();
  }
  
  // Check for re-enabling BLE after security timeout
  if (!bleEnabled && loadBool("ble_temp_disabled", false)) {
    unsigned long reenableTime = loadULong("ble_reenable_time", 0);
    if (millis() > reenableTime) {
      // Re-enable BLE
      saveBool("ble_temp_disabled", false);
      logInfo("BLE", "Re-enabling BLE after security timeout");
      bleEnabled = true;
      bleInit();
    }
  }
}

// Check if user ID is received
bool isUserIdReceived() {
  return userIdReceived;
}

// Get the user ID
String getUserId() {
  return userId;
}

// Enable or disable BLE
void enableBLE(bool enable) {
  bleEnabled = enable;
  if (enable) {
    if (!deviceConnected) {
      BLEDevice::startAdvertising();
      logInfo("BLE", "BLE enabled");
    }
  } else {
    BLEDevice::deinit(true);
    logInfo("BLE", "BLE disabled");
  }
}

// Check if BLE is enabled
bool isBLEEnabled() {
  return bleEnabled;
}

// Check if BLE is connected
bool isBLEConnected() {
  return deviceConnected;
}

// Check if BLE connection is authenticated
bool isBLEAuthenticated() {
  return bleAuthenticated;
}
