#include "storage.h"
#include "utils.h"

// Preferences instance
static Preferences preferences;
static bool preferencesInitialized = false;

// Initialize storage
void storageInit() {
  logInfo("STORAGE", "Initializing storage");
  
  // Open preferences in read/write mode
  if (preferences.begin("safety-bracelet", false)) {
    preferencesInitialized = true;
    logInfo("STORAGE", "Storage initialized successfully");
  } else {
    logError("STORAGE", "Failed to initialize storage");
  }
}

// Save user ID
bool saveUserId(const String& userId) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot save userId");
    return false;
  }
  
  size_t written = preferences.putString("user_id", userId);
  if (written > 0) {
    logInfo("STORAGE", "Saved userId: " + userId);
    return true;
  } else {
    logError("STORAGE", "Failed to save userId");
    return false;
  }
}

// Load user ID
String loadUserId() {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot load userId");
    return "";
  }
  
  String userId = preferences.getString("user_id", "");
  if (userId.length() > 0) {
    logInfo("STORAGE", "Loaded userId: " + userId);
  } else {
    logInfo("STORAGE", "No stored userId found");
  }
  
  return userId;
}

// Save emergency event
bool saveEmergencyEvent(const char* type, unsigned long timestamp) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot save emergency event");
    return false;
  }
  
  bool success = true;
  success &= preferences.putString("emg_type", type) > 0;
  success &= preferences.putULong("emg_time", timestamp) > 0;
  
  if (success) {
    logInfo("STORAGE", "Saved emergency event: " + String(type) + " at " + String(timestamp));
  } else {
    logError("STORAGE", "Failed to save emergency event");
  }
  
  return success;
}

// Get last emergency event
bool getLastEmergencyEvent(char* type, unsigned long* timestamp) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot load emergency event");
    return false;
  }
  
  String emgType = preferences.getString("emg_type", "");
  *timestamp = preferences.getULong("emg_time", 0);
  
  if (emgType.length() > 0 && *timestamp > 0) {
    strcpy(type, emgType.c_str());
    logInfo("STORAGE", "Loaded emergency event: " + emgType + " at " + String(*timestamp));
    return true;
  } else {
    logInfo("STORAGE", "No stored emergency event found");
    return false;
  }
}

// Save boolean value
bool saveBool(const char* key, bool value) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot save boolean");
    return false;
  }
  
  bool success = preferences.putBool(key, value);
  if (success) {
    logInfo("STORAGE", "Saved bool " + String(key) + ": " + String(value));
  } else {
    logError("STORAGE", "Failed to save bool " + String(key));
  }
  
  return success;
}

// Load boolean value
bool loadBool(const char* key, bool defaultValue) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot load boolean");
    return defaultValue;
  }
  
  bool value = preferences.getBool(key, defaultValue);
  logInfo("STORAGE", "Loaded bool " + String(key) + ": " + String(value));
  return value;
}

// Save float value
bool saveFloat(const char* key, float value) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot save float");
    return false;
  }
  
  bool success = preferences.putFloat(key, value);
  if (success) {
    logInfo("STORAGE", "Saved float " + String(key) + ": " + String(value));
  } else {
    logError("STORAGE", "Failed to save float " + String(key));
  }
  
  return success;
}

// Load float value
float loadFloat(const char* key, float defaultValue) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot load float");
    return defaultValue;
  }
  
  float value = preferences.getFloat(key, defaultValue);
  logInfo("STORAGE", "Loaded float " + String(key) + ": " + String(value));
  return value;
}

// Save string value
bool saveString(const char* key, const String& value) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot save string");
    return false;
  }
  
  size_t written = preferences.putString(key, value);
  if (written > 0) {
    logInfo("STORAGE", "Saved string " + String(key) + ": " + value);
    return true;
  } else {
    logError("STORAGE", "Failed to save string " + String(key));
    return false;
  }
}

// Load string value
String loadString(const char* key, const String& defaultValue) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot load string");
    return defaultValue;
  }
  
  String value = preferences.getString(key, defaultValue);
  logInfo("STORAGE", "Loaded string " + String(key) + ": " + value);
  return value;
}

// Save integer value
bool saveInt(const char* key, int value) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot save int");
    return false;
  }
  
  bool success = preferences.putInt(key, value);
  if (success) {
    logInfo("STORAGE", "Saved int " + String(key) + ": " + String(value));
  } else {
    logError("STORAGE", "Failed to save int " + String(key));
  }
  
  return success;
}

// Load integer value
int loadInt(const char* key, int defaultValue) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot load int");
    return defaultValue;
  }
  
  int value = preferences.getInt(key, defaultValue);
  logInfo("STORAGE", "Loaded int " + String(key) + ": " + String(value));
  return value;
}

// Save unsigned long value
bool saveULong(const char* key, unsigned long value) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot save ulong");
    return false;
  }
  
  bool success = preferences.putULong(key, value);
  if (success) {
    logInfo("STORAGE", "Saved ulong " + String(key) + ": " + String(value));
  } else {
    logError("STORAGE", "Failed to save ulong " + String(key));
  }
  
  return success;
}

// Load unsigned long value
unsigned long loadULong(const char* key, unsigned long defaultValue) {
  if (!preferencesInitialized) {
    logError("STORAGE", "Storage not initialized, cannot load ulong");
    return defaultValue;
  }
  
  unsigned long value = preferences.getULong(key, defaultValue);
  logInfo("STORAGE", "Loaded ulong " + String(key) + ": " + String(value));
  return value;
}
