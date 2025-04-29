#include "main.h"
#include "ble_manager.h"
#include "wifi_manager.h"
#include "sensors.h"
#include "display.h"
#include "emergency.h"
#include "power.h"
#include "storage.h"
#include "api.h"
#include "utils.h"

// Global state variables
bool apiInitialized = false;

// Main setup function
void mainSetup() {
  // Initialize serial communication
  Serial.begin(115200);
  logInfo("MAIN", "Safety bracelet initializing...");
  
  // Initialize modules in sequence
  storageInit();
  displayInit();
  displayLogo();
  
  // Load saved userId or start BLE
  String savedUserId = loadUserId();
  if (savedUserId.length() > 0) {
    logInfo("MAIN", "Using stored userId: " + savedUserId);
    apiInit(savedUserId);
    apiInitialized = true;
  } else {
    logInfo("MAIN", "No stored userId, initializing BLE");
    bleInit();
  }
  
  // Initialize sensors
  sensorsInit();
  
  // Check and run calibration if needed
  if (!isCalibrationComplete()) {
    logInfo("MAIN", "Running fall detection calibration");
    calibrateFallDetection();
  } else {
    logInfo("MAIN", "Using stored calibration values");
    loadCalibrationData();
  }
  
  // Initialize remaining modules
  emergencyInit();
  powerInit();
  wifiInit();
  
  // If WiFi fails, try GPRS
  if (getCurrentConnectionMode() == NO_CONNECTION && isSimModuleReady()) {
    logInfo("MAIN", "WiFi connection failed, trying GPRS");
    connectToGPRS();
  }
  
  // Fetch child data for QR code if network is connected
  if (isNetworkConnected() && apiInitialized) {
    logInfo("MAIN", "Fetching child data for QR code");
    String childData;
    fetchChildData(&childData);
  }
  
  // Setup OTA updates
  setupOTA();
  
  // Initialize watchdog timer
  initWatchdog();
  
  logInfo("MAIN", "Initialization complete");
}

// Main loop function
void mainLoop() {
  // Feed watchdog
  feedWatchdog();
  
  // Update non-blocking components
  updateBuzzer();
  
  // Process BLE if active
  if (isBLEEnabled()) {
    bleHandleEvents();
    
    // If we just received userId, initialize API
    if (isUserIdReceived() && !apiInitialized) {
      String userId = getUserId();
      saveUserId(userId);
      apiInit(userId);
      apiInitialized = true;
    }
  }
  
  // Check sensors
  checkGps();
  checkMPU();
  updateBatteryLevel();
  
  // Handle touch input for SOS and BLE toggle
  handleSOSTouch();
  handleBLETouch();
  
  // Check and maintain network connection
  checkConnection();
  
  // Update display
  updateDisplay();
  
  // Periodic tasks using non-blocking timing
  unsigned long currentTime = millis();
  
  // Send GPS data every 15 minutes
  static unsigned long lastGpsSendTime = 0;
  if (currentTime - lastGpsSendTime > GPS_SEND_INTERVAL && isGpsValid()) {
    if (sendGpsData(getLatitude(), getLongitude())) {
      lastGpsSendTime = currentTime;
    }
  }
  
  // Send battery status
  static unsigned long lastBatterySendTime = 0;
  if (currentTime - lastBatterySendTime > BATTERY_SEND_INTERVAL) {
    if (sendBatteryStatus(getBatteryPercentage())) {
      lastBatterySendTime = currentTime;
    }
  }
  
  // Update activity status
  updateActivity();
  
  // Handle OTA updates
  handleOTA();
  
  // Enter light sleep if conditions are met
  if (shouldEnterSleep()) {
    enterLightSleep();
  }
  
  // Allow background tasks to run
  yield();
}
