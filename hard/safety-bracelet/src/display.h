#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <qrcode.h>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Display page types
enum DisplayPage {
  STATUS_PAGE,      // Page 1: Bracelet Status (Internet, BLE, Battery)
  SENSOR_PAGE,      // Page 2: Sensor Status (MPU, SIM, GPS)
  QR_CODE_PAGE,     // Page 3: QR Code with emergency info
  INSTRUCTIONS_PAGE, // Page 4: User instructions
  NUM_PAGES
};

// Display functions
void displayInit();
void displayLogo();
void setDisplayPage(DisplayPage page);
void updateDisplay();
void rotateDisplayPage();
void displayStatusPage();
void displaySensorPage();
void displayQrCodePage();
void displayInstructionsPage();
void displayEmergencyMessage(const char* message);
void displayQRCode(const String& data);

#endif // DISPLAY_H
