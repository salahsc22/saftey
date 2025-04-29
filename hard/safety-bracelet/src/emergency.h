#ifndef EMERGENCY_H
#define EMERGENCY_H

#include <Arduino.h>

// Touch pins
#define SOS_TOUCH_PIN 27
#define BLE_TOGGLE_TOUCH_PIN 14
#define TOUCH_THRESHOLD 40
#define LONG_PRESS_DURATION 3000

// Buzzer pin
#define BUZZER_PIN 13

// Emergency functions
void emergencyInit();
bool sendEmergencyAlert(const char* title, const char* message, int priority);
bool sendSMS(const char* message);
bool makeEmergencyCall();
void handleSOSTouch();
void handleBLETouch();
bool isInEmergencyMode();
void activateBuzzer(int duration);
void updateBuzzer();

#endif // EMERGENCY_H
