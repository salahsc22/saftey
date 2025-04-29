#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS++.h>

// GPS settings
#define GPS_RX 16
#define GPS_TX 17
#define GPS_SEND_INTERVAL 900000  // 15 minutes

// Battery monitoring
#define BATTERY_PIN 34
#define BATTERY_LOW_THRESHOLD 30
#define BATTERY_SAMPLES 10
#define BATTERY_SEND_INTERVAL 60000  // 1 minute

// Fall detection
#define CALIBRATION_SAMPLES 500
#define CALIBRATION_THRESHOLD_MULTIPLIER 3.0
#define FALL_CONFIRMATION_DELAY 2000

// Functions
void sensorsInit();
void calibrateFallDetection();
void loadCalibrationData();
bool isCalibrationComplete();
void checkMPU();
bool isFallDetected();
void checkGps();
bool isGpsValid();
float getLatitude();
float getLongitude();
int getBatteryPercentage();
void updateBatteryLevel();

#endif // SENSORS_H
