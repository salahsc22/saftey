#include "emergency.h"
#include "utils.h"
#include "wifi_manager.h"
#include "display.h"
#include "sensors.h"
#include "api.h"
#include "storage.h"
#include "ble_manager.h"  // Added to get access to BLE functions

// Emergency state variables
static bool isEmergencyMode = false;

// SOS touch handling state
enum SOSTouchState {
  SOS_TOUCH_IDLE,
  SOS_TOUCH_ACTIVE,
  SOS_TOUCH_EMERGENCY,
  SOS_TOUCH_WAIT_RELEASE
};
static SOSTouchState sosTouchState = SOS_TOUCH_IDLE;
static unsigned long sosStateChangeTime = 0;

// BLE touch handling state
enum BLETouchState {
  BLE_TOUCH_IDLE,
  BLE_TOUCH_DETECTED,
  BLE_TOUCH_CONFIRMED,
  BLE_TOUCH_WAIT_RELEASE
};
static BLETouchState bleTouchState = BLE_TOUCH_IDLE;
static unsigned long bleTouchStateChangeTime = 0;

// Buzzer control variables
static unsigned long buzzerStartTime = 0;
static unsigned long buzzerDuration = 0;
static bool buzzerActive = false;

// Emergency contacts (loaded from storage)
static String emergencyContacts[5];
static int emergencyContactCount = 0;

// Initialize emergency system
void emergencyInit() {
  logInfo("EMERGENCY", "Initializing emergency system");
  
  // Initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Load emergency contacts from storage
  emergencyContactCount = 0;
  for (int i = 0; i < 5; i++) {
    String contactKeyStr = "contact_" + String(i);
    String contact = loadString(contactKeyStr.c_str(), "");
    if (contact.length() > 0) {
      emergencyContacts[emergencyContactCount++] = contact;
    }
  }
  
  if (emergencyContactCount == 0) {
    // Add a default emergency contact if none are saved
    emergencyContacts[0] = "+1234567890";  // Replace with actual default number
    emergencyContactCount = 1;
    saveString("contact_0", emergencyContacts[0]);
  }
  
  logInfo("EMERGENCY", "Emergency system initialized with " + String(emergencyContactCount) + " contacts");
}

// Send emergency alert through API
bool sendEmergencyAlert(const char* title, const char* message, int priority) {
  // First try to send notification via API
  bool success = sendNotification(title, message, priority);
  
  // If API fails and we have SIM module, try SMS
  if (!success && isSimModuleReady()) {
    String fullMessage = String(title) + ": " + String(message);
    if (isGpsValid()) {
      fullMessage += " Location: " + String(getLatitude(), 6) + "," + String(getLongitude(), 6);
    }
    
    success = sendSMS(fullMessage.c_str());
    
    // For high priority, make emergency call too
    if (priority >= 3) {
      makeEmergencyCall();
    }
  }
  
  // Log the result
  if (success) {
    logInfo("EMERGENCY", "Emergency alert sent successfully");
  } else {
    logError("EMERGENCY", "Failed to send emergency alert");
  }
  
  return success;
}

// Send SMS with improved reliability
bool sendSMS(const char* message) {
  if (!isSimModuleReady()) {
    logError("EMERGENCY", "SIM module not ready, cannot send SMS");
    return false;
  }
  
  logInfo("EMERGENCY", "Sending SMS alert");
  
  bool success = false;
  int attempts = 0;
  const int maxAttempts = 3;
  
  // Try each contact until one succeeds
  for (int i = 0; i < emergencyContactCount && !success; i++) {
    String phone = emergencyContacts[i];
    attempts = 0;
    
    while (!success && attempts < maxAttempts) {
      // Use a wrapper function to send SMS via SIM800L module
      logInfo("EMERGENCY", "Sending SMS to " + phone + ", attempt " + String(attempts + 1));
      
      // Call the sendSMSToNumber function from wifi_manager.cpp
      // This is a wrapper that handles the TinyGSM calls internally
      success = sendSMSToNumber(phone.c_str(), message);
      
      if (success) {
        logInfo("EMERGENCY", "SMS sent successfully to " + phone);
        break;
      } else {
        logError("EMERGENCY", "Failed to send SMS to " + phone);
      }
      
      attempts++;
      if (!success && attempts < maxAttempts) {
        delay(1000);  // Wait before retry
      }
    }
    
    if (success) break;  // Exit the contact loop if we've successfully sent to one
  }
  
  return success;
}

// Make emergency call with improved reliability
bool makeEmergencyCall() {
  if (emergencyContactCount == 0) {
    logError("EMERGENCY", "No emergency contacts set, cannot make call");
    return false;
  }
  
  // Try each contact until one succeeds
  for (int i = 0; i < emergencyContactCount; i++) {
    String phone = emergencyContacts[i];
    logInfo("EMERGENCY", "Calling emergency contact " + String(i+1) + ": " + phone);
    
    // Use the wrapper function to make the call
    if (makeCallToNumber(phone.c_str(), 30000)) { // 30 seconds call duration
      logInfo("EMERGENCY", "Emergency call successfully connected to " + phone);
      return true;
    }
    
    logError("EMERGENCY", "Failed to call " + phone + ", trying next contact if available");
    delay(2000); // Short delay before trying next contact
  }
  
  logError("EMERGENCY", "Failed to make emergency call to any contact");
  return false;
}

// Handle SOS touch sensor in a non-blocking way
void handleSOSTouch() {
  // Read touch sensor value
  int touchValue = touchRead(SOS_TOUCH_PIN);
  
  switch (sosTouchState) {
    case SOS_TOUCH_IDLE:
      if (touchValue < TOUCH_THRESHOLD) {
        // Touch detected, start timing
        sosTouchState = SOS_TOUCH_ACTIVE;
        sosStateChangeTime = millis();
        logInfo("EMERGENCY", "SOS touch detected");
      }
      break;
    
    case SOS_TOUCH_ACTIVE:
      if (touchValue >= TOUCH_THRESHOLD) {
        // Touch released before emergency trigger
        sosTouchState = SOS_TOUCH_IDLE;
        logInfo("EMERGENCY", "SOS touch released before emergency trigger");
      } else {
        // Check for long press duration
        unsigned long touchDuration = millis() - sosStateChangeTime;
        
        // Long press (3 seconds) triggers SOS
        if (touchDuration >= LONG_PRESS_DURATION) {
          // Move to emergency state
          sosTouchState = SOS_TOUCH_EMERGENCY;
          logInfo("EMERGENCY", "SOS emergency triggered after " + String(touchDuration) + "ms");
          
          // Display SOS message
          displayEmergencyMessage("SOS ALERT");
          
          // Set emergency mode
          isEmergencyMode = true;
          
          // Start alert sequence - non-blocking
          activateBuzzer(2000);
          
          // Save emergency event
          saveEmergencyEvent("SOS", millis());
        }
      }
      break;
    
    case SOS_TOUCH_EMERGENCY:
      // Send notifications, update location, etc.
      logInfo("EMERGENCY", "Sending SOS emergency notifications");
      
      // Send emergency alert
      sendEmergencyAlert("SOS ALERT", "Emergency button activated by user", 3);
      
      // Move to wait for release state
      sosTouchState = SOS_TOUCH_WAIT_RELEASE;
      break;
    
    case SOS_TOUCH_WAIT_RELEASE:
      // Wait for touch to be released
      if (touchValue >= TOUCH_THRESHOLD) {
        // Reset emergency mode when touch released
        isEmergencyMode = false;
        sosTouchState = SOS_TOUCH_IDLE;
        logInfo("EMERGENCY", "SOS touch released, emergency mode ended");
      }
      break;
  }
}

// Handle BLE toggle touch sensor in a non-blocking way
void handleBLETouch() {
  // Read touch sensor value
  int touchValue = touchRead(BLE_TOGGLE_TOUCH_PIN);
  
  switch (bleTouchState) {
    case BLE_TOUCH_IDLE:
      if (touchValue < TOUCH_THRESHOLD) {
        // Touch detected, move to next state
        bleTouchState = BLE_TOUCH_DETECTED;
        bleTouchStateChangeTime = millis();
        logInfo("EMERGENCY", "BLE toggle touch detected");
      }
      break;
      
    case BLE_TOUCH_DETECTED:
      // Debounce check (after 100ms)
      if (millis() - bleTouchStateChangeTime >= 100) {
        touchValue = touchRead(BLE_TOGGLE_TOUCH_PIN);
        if (touchValue < TOUCH_THRESHOLD) {
          // Touch confirmed
          bleTouchState = BLE_TOUCH_CONFIRMED;
          logInfo("EMERGENCY", "BLE toggle touch confirmed");
          
          // Toggle BLE state - call functions from ble_manager.h
          bool currentBleState = isBLEEnabled();
          enableBLE(!currentBleState);
          
          // Display confirmation
          if (!currentBleState) { // It's toggled, so check the inverse of previous state
            displayEmergencyMessage("BLE Enabled");
          } else {
            displayEmergencyMessage("BLE Disabled");
          }
          
          // Move to wait for release state
          bleTouchState = BLE_TOUCH_WAIT_RELEASE;
        } else {
          // False touch, go back to idle
          bleTouchState = BLE_TOUCH_IDLE;
        }
      }
      break;
      
    case BLE_TOUCH_CONFIRMED:
      // This state is transitional and handled above
      bleTouchState = BLE_TOUCH_WAIT_RELEASE;
      break;
      
    case BLE_TOUCH_WAIT_RELEASE:
      // Wait for touch release
      if (touchValue >= TOUCH_THRESHOLD) {
        // Touch released, back to idle after cooldown
        bleTouchStateChangeTime = millis();
        bleTouchState = BLE_TOUCH_IDLE;
        logInfo("EMERGENCY", "BLE toggle touch released");
      }
      break;
  }
}

// Check if in emergency mode
bool isInEmergencyMode() {
  return isEmergencyMode;
}

// Start buzzer for a specified duration (non-blocking)
void activateBuzzer(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  buzzerStartTime = millis();
  buzzerDuration = duration;
  buzzerActive = true;
  logInfo("EMERGENCY", "Buzzer activated for " + String(duration) + "ms");
}

// Update buzzer state (call this in main loop)
void updateBuzzer() {
  if (buzzerActive && (millis() - buzzerStartTime >= buzzerDuration)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
    logInfo("EMERGENCY", "Buzzer deactivated");
  }
}

// Set emergency contacts
void setEmergencyContacts(String* contacts, int count) {
  emergencyContactCount = min(count, 5);  // Maximum 5 contacts
  
  for (int i = 0; i < emergencyContactCount; i++) {
    emergencyContacts[i] = contacts[i];
    
    // Save to storage
    String contactKeyStr = "contact_" + String(i);
    saveString(contactKeyStr.c_str(), contacts[i]);
  }
  
  logInfo("EMERGENCY", "Saved " + String(emergencyContactCount) + " emergency contacts");
}
