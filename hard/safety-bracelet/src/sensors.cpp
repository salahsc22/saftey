#include "sensors.h"
#include "utils.h"
#include "storage.h"
#include "api.h"  // Added to get access to sendNotification function

// Hardware instances
static Adafruit_MPU6050 mpu;
static HardwareSerial gpsSerial(1);
static TinyGPSPlus gps;

// MPU6050 status
static bool mpuInitialized = false;
static bool fallDetected = false;
static unsigned long fallDetectionTime = 0;
static bool calibrationComplete = false;

// Calibration data
static float baselineAccel[3] = {0, 0, 0};
static float baselineVariance[3] = {0, 0, 0};
static float dynamicFallThreshold = 2.0;
static sensors_event_t accel, gyro, temp;

// GPS data
static float latitude = 0.0;
static float longitude = 0.0;
static bool gpsValid = false;

// Battery data
static int batteryPercentage = 100;
static bool batteryAlertSent = false;

// Initialize all sensors
void sensorsInit() {
  logInfo("SENSORS", "Initializing sensors");
  
  // Initialize MPU6050
  Wire.begin();
  if (mpu.begin()) {
    mpuInitialized = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    logInfo("SENSORS", "MPU6050 initialized successfully");
  } else {
    logError("SENSORS", "Failed to find MPU6050 chip");
  }
  
  // Initialize GPS
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  logInfo("SENSORS", "GPS initialized");
  
  // Get calibration status
  calibrationComplete = loadBool("cal_complete", false);
}

// Run calibration for fall detection
void calibrateFallDetection() {
  if (!mpuInitialized) {
    logError("SENSORS", "MPU not initialized, cannot calibrate");
    return;
  }
  
  logInfo("SENSORS", "Starting fall detection calibration...");
  
  // Data for collecting samples
  float sampleX[CALIBRATION_SAMPLES];
  float sampleY[CALIBRATION_SAMPLES];
  float sampleZ[CALIBRATION_SAMPLES];
  
  // Collect samples
  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);
    
    sampleX[i] = accel.acceleration.x;
    sampleY[i] = accel.acceleration.y;
    sampleZ[i] = accel.acceleration.z;
    
    // Short delay between readings
    delay(10);
  }
  
  // Calculate mean
  float sumX = 0, sumY = 0, sumZ = 0;
  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    sumX += sampleX[i];
    sumY += sampleY[i];
    sumZ += sampleZ[i];
  }
  
  baselineAccel[0] = sumX / CALIBRATION_SAMPLES;
  baselineAccel[1] = sumY / CALIBRATION_SAMPLES;
  baselineAccel[2] = sumZ / CALIBRATION_SAMPLES;
  
  // Calculate variance
  sumX = 0; sumY = 0; sumZ = 0;
  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    sumX += pow(sampleX[i] - baselineAccel[0], 2);
    sumY += pow(sampleY[i] - baselineAccel[1], 2);
    sumZ += pow(sampleZ[i] - baselineAccel[2], 2);
  }
  
  baselineVariance[0] = sumX / CALIBRATION_SAMPLES;
  baselineVariance[1] = sumY / CALIBRATION_SAMPLES;
  baselineVariance[2] = sumZ / CALIBRATION_SAMPLES;
  
  // Calculate dynamic threshold based on baseline
  float maxVariance = max(max(baselineVariance[0], baselineVariance[1]), baselineVariance[2]);
  dynamicFallThreshold = sqrt(maxVariance) * CALIBRATION_THRESHOLD_MULTIPLIER;
  
  // Ensure minimum threshold
  if (dynamicFallThreshold < 1.5) dynamicFallThreshold = 1.5;
  
  logInfo("SENSORS", "Calibration complete. Dynamic threshold: " + String(dynamicFallThreshold));
  calibrationComplete = true;
  
  // Save calibration to storage
  saveFloat("fall_thresh", dynamicFallThreshold);
  saveFloat("base_x", baselineAccel[0]);
  saveFloat("base_y", baselineAccel[1]);
  saveFloat("base_z", baselineAccel[2]);
  saveFloat("var_x", baselineVariance[0]);
  saveFloat("var_y", baselineVariance[1]);
  saveFloat("var_z", baselineVariance[2]);
  saveBool("cal_complete", true);
}

// Load calibration data from storage
void loadCalibrationData() {
  dynamicFallThreshold = loadFloat("fall_thresh", 2.0);
  baselineAccel[0] = loadFloat("base_x", 0);
  baselineAccel[1] = loadFloat("base_y", 0);
  baselineAccel[2] = loadFloat("base_z", 0);
  baselineVariance[0] = loadFloat("var_x", 0);
  baselineVariance[1] = loadFloat("var_y", 0);
  baselineVariance[2] = loadFloat("var_z", 0);
  
  logInfo("SENSORS", "Loaded fall threshold: " + String(dynamicFallThreshold));
}

// Check if calibration is complete
bool isCalibrationComplete() {
  return calibrationComplete;
}

// Check MPU for fall detection
void checkMPU() {
  if (!mpuInitialized || !calibrationComplete) {
    return;
  }
  
  // Get new sensor readings
  mpu.getEvent(&accel, &gyro, &temp);
  
  // Static variables for the fall detection algorithm
  static bool freeFallDetected = false;
  static bool impactDetected = false;
  static bool orientationChanged = false;
  static unsigned long freeFallTime = 0;
  static unsigned long impactTime = 0;
  static float previousAccelMagnitude = 0;
  static float previousOrientation = 0;
  static float impactPeakMagnitude = 0;
  static float accelMagnitudeBuffer[10] = {0};
  static int bufferIndex = 0;
  
  // Calculate total acceleration magnitude (vector sum)
  float accelMagnitude = sqrt(accel.acceleration.x * accel.acceleration.x + 
                             accel.acceleration.y * accel.acceleration.y + 
                             accel.acceleration.z * accel.acceleration.z);
  
  // Buffer for smoothing
  accelMagnitudeBuffer[bufferIndex] = accelMagnitude;
  bufferIndex = (bufferIndex + 1) % 10;
  
  // Calculate current orientation
  float currentOrientation = atan2(accel.acceleration.z, 
                              sqrt(accel.acceleration.x * accel.acceleration.x + 
                                   accel.acceleration.y * accel.acceleration.y)) * 180.0 / PI;
  
  // Calculate current movement level (jitter)
  float currentMovement = abs(gyro.gyro.x) + abs(gyro.gyro.y) + abs(gyro.gyro.z);
  
  // Calculate rate of change of acceleration
  float accelRateOfChange = accelMagnitude - previousAccelMagnitude;
  previousAccelMagnitude = accelMagnitude;
  
  // STEP 1: Detect free fall (acceleration < 0.4G for a short time)
  if (!freeFallDetected && accelMagnitude < 0.4 * SENSORS_GRAVITY_STANDARD) {
    freeFallDetected = true;
    freeFallTime = millis();
    logInfo("SENSORS", "Free fall detected: " + String(accelMagnitude));
  }
  
  // STEP 2: Detect impact after free fall
  if (freeFallDetected && !impactDetected && 
      millis() - freeFallTime < 500 && // Impact must occur within 500ms of free fall
      accelMagnitude > dynamicFallThreshold * SENSORS_GRAVITY_STANDARD) {
    
    impactDetected = true;
    impactTime = millis();
    impactPeakMagnitude = accelMagnitude;
    logInfo("SENSORS", "Impact detected after free fall: " + String(accelMagnitude));
  }
  
  // STEP 3: Track peak impact
  if (impactDetected && millis() - impactTime < 100) {
    if (accelMagnitude > impactPeakMagnitude) {
      impactPeakMagnitude = accelMagnitude;
    }
  }
  
  // STEP 4: Check for orientation change after impact
  if (impactDetected && !orientationChanged && 
      millis() - impactTime > 100 && millis() - impactTime < 1000) {
    
    float orientationChange = abs(currentOrientation - previousOrientation);
    if (orientationChange > 30.0) {
      orientationChanged = true;
      logInfo("SENSORS", "Orientation change detected: " + String(orientationChange));
    }
  }
  
  // STEP 5: Final fall confirmation after delay
  if (impactDetected && millis() - impactTime > FALL_CONFIRMATION_DELAY && !fallDetected) {
    // Check if the person is still (minimal movement)
    if (currentMovement < 0.2) {
      // Check all fall criteria
      if (freeFallDetected && orientationChanged && 
          impactPeakMagnitude > (dynamicFallThreshold * SENSORS_GRAVITY_STANDARD)) {
        
        logInfo("SENSORS", "Fall confirmed! Person is likely unconscious or immobile. Impact: " + 
                String(impactPeakMagnitude) + ", Movement: " + String(currentMovement));
        
        // Save fall event to storage with severity level
        float severity = min(10.0f, impactPeakMagnitude / SENSORS_GRAVITY_STANDARD);
        String eventData = "FALL:SEV:" + String(severity, 1);
        saveEmergencyEvent(eventData.c_str(), millis());
        
        // Set fall detected flag to trigger emergency protocol
        fallDetected = true;
      } else {
        logInfo("SENSORS", "Fall criteria not fully met - possible false alarm");
      }
    }
    
    // Reset all detection flags for next cycle
    freeFallDetected = false;
    impactDetected = false;
    orientationChanged = false;
    impactPeakMagnitude = 0;
  }
  
  // Reset after a certain time if the fall sequence wasn't completed
  if ((freeFallDetected || impactDetected) && millis() - freeFallTime > 3000) {
    freeFallDetected = false;
    impactDetected = false;
    orientationChanged = false;
    impactPeakMagnitude = 0;
    logInfo("SENSORS", "Fall detection sequence timeout - resetting flags");
  }
  
  // Store previous orientation for comparison
  previousOrientation = currentOrientation;
}

// Check if fall is detected
bool isFallDetected() {
  if (fallDetected) {
    // Reset fallDetected flag after reporting it once
    fallDetected = false;
    return true;
  }
  return false;
}

// Check GPS data
void checkGps() {
  // Process GPS data while available
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      // Update GPS data if it's valid
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
        gpsValid = true;
        
        // Log position occasionally (avoid log spam)
        static unsigned long lastGpsLogTime = 0;
        if (millis() - lastGpsLogTime > 60000) {  // Log every minute
          logInfo("SENSORS", "GPS Position: " + String(latitude, 6) + ", " + String(longitude, 6));
          lastGpsLogTime = millis();
        }
      } else {
        gpsValid = false;
      }
    }
  }
  
  // Check if GPS is stuck - recommend adding a GPS timeout check here
}

// Check if GPS has valid data
bool isGpsValid() {
  return gpsValid;
}

// Get current latitude
float getLatitude() {
  return latitude;
}

// Get current longitude
float getLongitude() {
  return longitude;
}

// Get battery percentage
int getBatteryPercentage() {
  return batteryPercentage;
}

// Update battery level with advanced monitoring and calibration
void updateBatteryLevel() {
  // Read battery voltage multiple times and average for smoothing
  int rawTotal = 0;
  for (int i = 0; i < BATTERY_SAMPLES; i++) {
    rawTotal += analogRead(BATTERY_PIN);
    delay(5);  // Short delay between readings
  }
  
  int rawAverage = rawTotal / BATTERY_SAMPLES;
  
  // Load calibration constants (allow for calibration adjustment)
  static float voltageCalibration = loadFloat("batt_cal", 1.0);
  static float minVoltage = loadFloat("batt_min", 3.3);
  static float maxVoltage = loadFloat("batt_max", 4.2);
  
  // Convert to voltage with calibration factor
  float voltage = (rawAverage * 3.3 / 4095.0) * voltageCalibration;
  
  // Apply voltage divider formula with 100k and 150k resistors from TP4056
  // To calculate the real battery voltage using the resistors:
  // The voltage divider factor is 100k/(100k+150k) = 0.4
  // So the multiplier to convert from measured voltage to actual battery voltage is 1/0.4 = 2.5
  static float voltageDividerRatio = loadFloat("volt_div", 2.5); // Factor: 1/(100k/(100k+150k)) = 2.5
  float batteryVoltage = voltage * voltageDividerRatio;
  
  // Add TP4056 specific offset if needed - the TP4056 might have voltage drop
  static float voltageOffset = loadFloat("volt_offset", 0.0);
  batteryVoltage += voltageOffset;
  
  // Use a non-linear mapping for Li-ion batteries
  // Li-ion discharge is not linear, this curve better approximates real discharge behavior
  int percentage;
  
  if (batteryVoltage >= maxVoltage) {
    percentage = 100;
  } else if (batteryVoltage <= minVoltage) {
    percentage = 0;
  } else {
    // Non-linear mapping for Li-ion batteries (more accurate than linear mapping)
    float normalizedVoltage = (batteryVoltage - minVoltage) / (maxVoltage - minVoltage);
    
    // This curve better approximates Li-ion discharge behavior
    // Gives more realistic values in the 20-80% range where Li-ion discharge is more stable
    if (normalizedVoltage > 0.9f) {
      percentage = 90 + (normalizedVoltage - 0.9f) * 100;
    } else if (normalizedVoltage > 0.7f) {
      percentage = 70 + (normalizedVoltage - 0.7f) * 100;
    } else if (normalizedVoltage > 0.4f) {
      percentage = 40 + (normalizedVoltage - 0.4f) * 100;
    } else if (normalizedVoltage > 0.2f) {
      percentage = 20 + (normalizedVoltage - 0.2f) * 100;
    } else {
      percentage = normalizedVoltage * 100;
    }
  }
  
  // Ensure percentage is within valid range
  percentage = constrain(percentage, 0, 100);
  
  // Apply advanced smoothing using exponential moving average
  static float smoothedPercentage = -1;
  if (smoothedPercentage < 0) {
    smoothedPercentage = percentage; // Initialize on first run
  } else {
    // Apply more weight to current reading when changing fast, less weight when stable
    float alpha = abs(smoothedPercentage - percentage) > 5 ? 0.3 : 0.1;
    smoothedPercentage = (alpha * percentage) + ((1 - alpha) * smoothedPercentage);
  }
  
  // Round to nearest integer
  batteryPercentage = round(smoothedPercentage);
  
  // Log battery status if significant change
  static int lastReportedPercentage = -1;
  if (abs(lastReportedPercentage - batteryPercentage) >= 3 || lastReportedPercentage == -1) {
    logInfo("SENSORS", "Battery: " + String(batteryPercentage) + "% (" + String(batteryVoltage, 2) + "V raw:" + String(rawAverage) + ")");
    lastReportedPercentage = batteryPercentage;
  }
  
  // Multi-level battery alerts
  if (batteryPercentage <= 10 && !batteryAlertSent) {
    logWarning("SENSORS", "CRITICAL battery warning: " + String(batteryPercentage) + "%");
    String criticalMsg = "Battery level critical at " + String(batteryPercentage) + "%, please charge immediately!";
    sendNotification("Critical Battery", criticalMsg.c_str(), 3);
    batteryAlertSent = true;
  } else if (batteryPercentage <= BATTERY_LOW_THRESHOLD && !batteryAlertSent) {
    logWarning("SENSORS", "Low battery warning: " + String(batteryPercentage) + "%");
    String lowBattMsg = "Battery level is at " + String(batteryPercentage) + "%";
    sendNotification("Low Battery", lowBattMsg.c_str(), 2);
    batteryAlertSent = true;
  } else if (batteryPercentage > BATTERY_LOW_THRESHOLD + 10) {
    // Reset alert flag when battery level improves significantly
    batteryAlertSent = false;
  }
  
  // Auto-calibration mode when charging is detected
  static bool wasCharging = false;
  bool isCharging = (batteryVoltage > maxVoltage - 0.05) && (smoothedPercentage < 98);
  
  if (isCharging && !wasCharging) {
    logInfo("SENSORS", "Charging detected - monitoring for calibration");
  } else if (!isCharging && wasCharging && batteryVoltage > 4.1) {
    // If we just stopped charging and voltage is high, assume we're at max charge
    // Update the calibration
    maxVoltage = batteryVoltage;
    saveFloat("batt_max", maxVoltage);
    logInfo("SENSORS", "Battery calibration updated - new max voltage: " + String(maxVoltage, 3) + "V");
  }
  
  wasCharging = isCharging;
}
