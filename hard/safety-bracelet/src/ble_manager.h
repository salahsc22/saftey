#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE function declarations
void bleInit();
void bleHandleEvents();
bool isBLEEnabled();
void enableBLE(bool enable);

// BLE UUIDs - Using standard UART service
#define SERVICE_UUID             "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_CHARACTERISTIC_UUID   "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // Notify from device to app
#define RX_CHARACTERISTIC_UUID   "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // Write from app to device
#define AUTH_CHARACTERISTIC_UUID "6E400004-B5A3-F393-E0A9-E50E24DCCA9E" // Auth and security

// Forward declarations of functions from other modules that BLE uses
// Instead of using extern "C", we include the necessary headers directly
#include "sensors.h"  // For getBatteryPercentage and isGpsValid

// BLE functions
void bleInit();
void bleHandleEvents();
bool isUserIdReceived();
String getUserId();
void enableBLE(bool enable);
bool isBLEEnabled();
bool isBLEConnected();
void updateDeviceName();
bool isBLEAuthenticated();

#endif // BLE_MANAGER_H
