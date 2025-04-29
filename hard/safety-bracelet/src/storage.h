#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <Preferences.h>

// Storage functions
void storageInit();
bool saveUserId(const String& userId);
String loadUserId();
bool saveEmergencyEvent(const char* type, unsigned long timestamp);
bool getLastEmergencyEvent(char* type, unsigned long* timestamp);
bool saveBool(const char* key, bool value);
bool loadBool(const char* key, bool defaultValue);
bool saveFloat(const char* key, float value);
float loadFloat(const char* key, float defaultValue);
bool saveString(const char* key, const String& value);
String loadString(const char* key, const String& defaultValue);
bool saveInt(const char* key, int value);
int loadInt(const char* key, int defaultValue);
bool saveULong(const char* key, unsigned long value);
unsigned long loadULong(const char* key, unsigned long defaultValue);

#endif // STORAGE_H
