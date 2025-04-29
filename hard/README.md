# Safety Bracelet - ESP32 Based Emergency Alert System

## Project Overview
The Safety Bracelet is an ESP32-based wearable device designed to provide continuous monitoring and emergency assistance for vulnerable individuals such as children, elderly, or patients with specific medical conditions. It combines GPS tracking, fall detection, and emergency alerting in a compact wearable form factor with multiple connectivity options.

## Table of Contents
- [Hardware Components](#hardware-components)
- [Pin Connections](#pin-connections)
- [Software Architecture](#software-architecture)
- [Features](#features)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage Instructions](#usage-instructions)
- [Display Interface](#display-interface)
- [Testing Scenarios](#testing-scenarios)
- [Serial Monitor Output](#serial-monitor-output)
- [Manual Implementation Requirements](#manual-implementation-requirements)
- [API Endpoints](#api-endpoints)
- [Troubleshooting](#troubleshooting)

## Hardware Components
- **ESP32 Development Board** - Main microcontroller
- **SSD1306 OLED Display** (128x64) - User interface
- **MPU6050** - Accelerometer/Gyroscope for fall detection
- **NEO-6M GPS Module** - Location tracking
- **SIM800L GSM Module** - For GPRS internet, SMS, and calls
- **Touch Sensors** - For user interaction (SOS and BLE toggle)
- **Buzzer** - For audible alerts
- **Battery** - LiPo battery with voltage divider for monitoring
- **Optional: Buttons, LEDs** - For additional interaction

## Pin Connections

### SSD1306 OLED Display
- SDA → ESP32 GPIO 21
- SCL → ESP32 GPIO 22
- VCC → 3.3V
- GND → GND

### MPU6050 Accelerometer
- SDA → ESP32 GPIO 21 (shared I2C bus)
- SCL → ESP32 GPIO 22 (shared I2C bus)
- VCC → 3.3V
- GND → GND
- INT → ESP32 GPIO 15 (for interrupt-based detection)

### NEO-6M GPS Module
- TX → ESP32 GPIO 16 (RX)
- RX → ESP32 GPIO 17 (TX)
- VCC → 3.3V
- GND → GND

### SIM800L GSM Module
- TX → ESP32 GPIO 26 (not explicitly defined in code, verify)
- RX → ESP32 GPIO 25 (not explicitly defined in code, verify)
- VCC → External 4.2V power supply (not 3.3V)
- GND → GND
- RESET → ESP32 GPIO 5 (if needed)

### Touch Pins
- SOS Button → ESP32 Touch Pin 27
- BLE Toggle → ESP32 Touch Pin 14

### Buzzer
- Positive → ESP32 GPIO 13
- Negative → GND

### Battery Monitoring
- Battery Voltage Divider Output → ESP32 GPIO 34 (ADC)

## Software Architecture
The project follows a modular architecture where functionality is separated into different components:

1. **Main Module** (`main.cpp`, `main.h`) - Program entry point and main loop
2. **BLE Manager** (`ble_manager.cpp`, `ble_manager.h`) - Bluetooth Low Energy functionality for app connection
3. **WiFi Manager** (`wifi_manager.cpp`, `wifi_manager.h`) - WiFi connectivity and management
4. **Sensors** (`sensors.cpp`, `sensors.h`) - GPS, accelerometer, and battery monitoring
5. **Display** (`display.cpp`, `display.h`) - OLED display interface with multiple pages
6. **Emergency** (`emergency.cpp`, `emergency.h`) - Emergency detection and alerting
7. **Power** (`power.cpp`, `power.h`) - Power management and sleep modes
8. **Storage** (`storage.cpp`, `storage.h`) - Persistent storage using ESP32 preferences
9. **API** (`api.cpp`, `api.h`) - Communication with backend server
10. **Utils** (`utils.cpp`, `utils.h`) - Utility functions and logging

## Features
- **User Identification**: BLE pairing with mobile app to retrieve user ID
- **Multi-page Display Interface**: Four navigable information pages
- **Location Tracking**: Continuous GPS monitoring and periodic location uploads
- **Fall Detection**: Automatic detection using accelerometer data
- **Emergency Alerts**: Manual (touch) and automatic (fall) triggers
- **Multi-channel Notifications**: API, SMS, and voice calls
- **Dual Connectivity**: WiFi with GPRS fallback
- **Battery Monitoring**: Level tracking and low battery alerts
- **Power Saving**: Light sleep mode when inactive
- **OTA Updates**: Remote firmware updating
- **QR Code Display**: Emergency information displayed as scannable QR code

## Installation
1. **Setup PlatformIO**:
   - Install Visual Studio Code and PlatformIO extension
   - Open project folder in PlatformIO
   
2. **Install Dependencies**:
   Uncomment the following libraries in `platformio.ini`:
   ```ini
   lib_deps =
     adafruit/Adafruit SSD1306@^2.5.7
     adafruit/Adafruit MPU6050@^2.2.4
     adafruit/Adafruit GPS Library@^1.7.3
     plerup/EspSoftwareSerial@^8.1.0
     ricmoo/QRCode@^0.0.1
   ```

3. **Hardware Setup**:
   - Connect all components according to the pin mapping
   - Ensure proper power supply (voltage regulator may be needed for SIM800L)

4. **Build and Upload**:
   - Connect ESP32 to computer via USB
   - Build and upload using PlatformIO

## Configuration
The following configuration parameters can be adjusted in the header files:

### WiFi Configuration (`wifi_manager.h`)
- Update SSID and password for your WiFi network

### SIM Configuration (if using GSM services)
- Update APN settings for your cellular provider
- Update emergency contact phone numbers

### API Configuration (`api.h`)
- The device communicates with backend services at `http://16.170.159.206:8000/`
- No changes needed unless the server address changes

## Usage Instructions
1. **Initial Setup**:
   - Power on the bracelet
   - The display will show the logo and initialization progress
   - If no user ID is stored, the device will enter BLE pairing mode
   - Connect with mobile app to provide user ID
   - Once paired, the user ID is stored for future use

2. **Normal Operation**:
   - The device continuously monitors location, motion, and battery
   - The display shows different pages that can be rotated automatically
   - Touch sensors can be used for emergency triggering or BLE toggling
   - Data is periodically sent to the backend server

3. **Emergency Activation**:
   - **Automatic**: Falls are detected using the MPU6050 accelerometer
   - **Manual**: Long-press the SOS touch sensor (Pin 27) for 3 seconds
   - When activated, the device will:
     - Sound the buzzer
     - Display emergency message
     - Send alert via API
     - Send SMS (if GSM module available)
     - Make emergency call (if GSM module available)

4. **Charging**:
   - Connect to USB charger
   - Monitor charging status on display
   - Full charge is indicated on the status page

## Display Interface
The display rotates through four pages:

1. **Status Page**:
   - WiFi/GPRS connection status
   - BLE status (on/off)
   - Battery percentage
   - Current time (if available)

2. **Sensor Page**:
   - GPS status and coordinates
   - Accelerometer status
   - SIM module status
   - Motion activity status

3. **QR Code Page**:
   - Displays QR code with emergency information
   - Contains user ID and quick access link

4. **Instructions Page**:
   - Brief usage instructions
   - Touch sensor guidance

## Testing Scenarios

### 1. Initial Setup Test
1. Power on the device with no stored user ID
2. Verify BLE advertisement is active
3. Connect with mobile app and send user ID
4. Verify the device accepts and stores the ID
5. Verify automatic connection to WiFi or GPRS

**Expected Result**: Device successfully stores user ID and establishes network connection

### 2. Fall Detection Test
1. Ensure the device is calibrated for fall detection
2. Simulate a fall motion
3. Wait for the fall confirmation delay (2 seconds)

**Expected Result**: Buzzer activates, emergency message displays, and alerts are sent

### 3. Manual Emergency Test
1. Long-press the SOS touch sensor (Pin 27) for 3+ seconds
2. Monitor the display and buzzer

**Expected Result**: Same emergency sequence as fall detection

### 4. GPS Tracking Test
1. Take the device outdoors for GPS signal
2. Wait for GPS fix (indicated on sensor page)
3. Verify coordinates are displayed correctly
4. Check backend server for location updates

**Expected Result**: Accurate location data displayed and uploaded periodically

### 5. Battery Monitoring Test
1. Monitor battery percentage on status page
2. Disconnect charger if connected
3. Wait until battery level drops below threshold (30%)

**Expected Result**: Low battery alert shown and battery status uploaded to server

### 6. Connection Fallback Test
1. Connect to WiFi initially
2. Move out of WiFi range or disable network
3. Verify fallback to GPRS (if SIM module present)

**Expected Result**: Seamless transition to backup connection method

## Serial Monitor Output
During normal operation, the serial monitor (115200 baud) will show diagnostic information:

```
[INFO][MAIN] Safety bracelet initializing...
[INFO][STORAGE] Preferences initialized
[INFO][DISPLAY] OLED initialized
[INFO][MAIN] Using stored userId: USR12345
[INFO][API] API initialized for user: USR12345
[INFO][SENSORS] Initializing sensors...
[INFO][SENSORS] GPS module initialized
[INFO][SENSORS] MPU6050 initialized
[INFO][MAIN] Using stored calibration values
[INFO][EMERGENCY] Emergency system initialized
[INFO][POWER] Power monitoring initialized
[INFO][WIFI] Connecting to WiFi...
[INFO][WIFI] WiFi connected, IP: 192.168.1.100
[INFO][MAIN] Fetching child data for QR code
[INFO][MAIN] Initialization complete

[INFO][SENSORS] GPS valid: lat=51.5074, lon=-0.1278
[INFO][API] GPS data sent successfully
[INFO][SENSORS] Battery level: 87%
[INFO][API] Battery status sent successfully
```

During emergency situations:
```
[INFO][SENSORS] Fall detected! Awaiting confirmation...
[INFO][EMERGENCY] Fall confirmed, triggering emergency protocol
[INFO][EMERGENCY] Sending emergency alert: "Fall detected"
[INFO][API] Emergency notification sent
[INFO][EMERGENCY] Sending SMS
[INFO][EMERGENCY] Making emergency call
```

## Manual Implementation Requirements
The following aspects need to be completed or verified manually:

1. **GSM Module Integration**:
   - Verify SIM800L pin connections
   - Implement proper power management for SIM800L (it requires 4.2V)
   - Test SMS and call functionality
   
2. **Battery Circuit**:
   - Implement voltage divider for battery monitoring
   - Calibrate ADC readings to accurate percentage values
   
3. **Fall Detection Calibration**:
   - Tune detection sensitivity based on testing
   - Adjust thresholds in `sensors.h` if needed
   
4. **Mobile App Connection**:
   - Develop or modify mobile app for BLE pairing
   - Implement user ID generation and transfer

5. **Physical Construction**:
   - Design compact enclosure
   - Ensure proper battery placement and charging access
   - Make touch sensors accessible on exterior

## API Endpoints
The device communicates with the following API endpoints:

- **Save Latitude**: `http://16.170.159.206:8000/save-latitude/`
- **Save Longitude**: `http://16.170.159.206:8000/save-longitude/userId/`
- **Add Notification**: `http://16.170.159.206:8000/add-notification/userId/`
- **Retrieve Child Data**: `http://16.170.159.206:8000/child_data/userId/`
- **Save Battery Status**: `http://16.170.159.206:8000/save-battery-status/userId/`

## Troubleshooting
- **Device not connecting to WiFi**: Check SSID and password in config, verify signal strength
- **GPS not getting fix**: Ensure outdoor usage or clear view of sky, check wiring
- **BLE not advertising**: Verify BLE is enabled and not in sleep mode
- **Fall detection too sensitive/not sensitive**: Adjust thresholds and re-calibrate
- **Battery reading incorrect**: Calibrate the voltage divider and ADC readings
- **API calls failing**: Check network connection and verify server is operational
- **Display not working**: Verify I2C connections and address (0x3C)
- **SIM module not responding**: Check power supply (needs 4.2V) and serial connections
