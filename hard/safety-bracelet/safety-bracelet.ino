/*
 * ESP32 Safety Bracelet - Main Entry Point
 * 
 * This file is the main entry point for the Arduino IDE.
 * Actual implementation is modularized in the src/ directory.
 */

// All functionality is included from modules
#include "src/main.h"

// Arduino setup function (calls the main setup in main.cpp)
void setup() {
  mainSetup();
}

// Arduino loop function (calls the main loop in main.cpp)
void loop() {
  mainLoop();
}
