// Auto-rotation using IMU

#ifndef AUTO_ROTATION_H
#define AUTO_ROTATION_H

#include <Arduino.h>
#include <FastIMU.h>

// IMU objects
extern QMI8658 imu;
extern calData imuCalibration;
extern AccelData accelData;
extern bool imuInitialized;

// Functions
uint8_t detectRotation();
void applyRotation(uint8_t newRotation);
void checkAutoRotation();

#endif // AUTO_ROTATION_H
