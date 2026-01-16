// Auto-rotation using IMU implementation

#include "auto_rotation.h"
#include "pomodoro_globals.h"
#include "pomodoro_config.h"
#include "display_graphics.h"
#include "display_updates.h"
#include <Wire.h>
#include "esp_lcd_touch_axs5106l.h"

// IMU objects
QMI8658 imu;
calData imuCalibration = { 0 };
AccelData accelData;
bool imuInitialized = false;

// Detect rotation based on accelerometer data (gravity direction)
uint8_t detectRotation() {
  if (!imuInitialized) return currentRotation;
  
  // IMU shares I2C bus with touch - no pin switching needed
  imu.update();
  imu.getAccel(&accelData);
  
  float ax = accelData.accelX;
  float ay = accelData.accelY;
  
  // Determine orientation based on which axis feels gravity
  // Portrait: Y-axis dominant, Landscape: X-axis dominant
  if (ay < -ROTATION_THRESHOLD) {
    return 0;  // Portrait normal (USB connector down)
  } else if (ay > ROTATION_THRESHOLD) {
    return 2;  // Portrait upside down (USB connector up)
  } else if (ax > ROTATION_THRESHOLD) {
    return 1;  // Landscape right
  } else if (ax < -ROTATION_THRESHOLD) {
    return 3;  // Landscape left
  }
  
  return currentRotation;  // Keep current if no clear orientation
}

// Apply new rotation to display and touch
void applyRotation(uint8_t newRotation) {
  if (newRotation == currentRotation) return;
  
  Serial.print("Rotation changed: ");
  Serial.print(currentRotation);
  Serial.print(" -> ");
  Serial.println(newRotation);
  
  currentRotation = newRotation;
  gfx->setRotation(currentRotation);
  
  // Re-initialize touch controller with new rotation
  bsp_touch_init(&Wire, TP_RST, TP_INT, gfx->getRotation(), gfx->width(), gfx->height());
  
  // Force full display refresh
  displayInitialized = false;
  forceCircleRedraw = true;  // Reset progress circle state
  memset(lastTimeStr, 0, sizeof(lastTimeStr));
  
  // Redraw current screen based on current view mode
  if (currentViewMode == VIEW_MODE_PREVIEW) {
    // Color preview screen - just redraw it
    drawColorPreview();
  } else if (currentState == STOPPED && currentViewMode == VIEW_MODE_HOME) {
    // Home screen
    drawSplash();
  } else if (currentViewMode == VIEW_MODE_GRID || gridViewActive) {
    // Grid/palette view
    drawGrid();
  } else if (currentViewMode == VIEW_MODE_MAIN_MENU) {
    // Main menu screen - redraw it
    drawMainFunctionality();
  } else if (currentViewMode == VIEW_MODE_B24) {
    // B24 screen - redraw it
    drawB24Placeholder();
  } else {
    // Timer screen
    gfx->fillScreen(COLOR_BLACK);
    drawTimer();
  }
}

// Check and handle auto-rotation (called from loop)
void checkAutoRotation() {
  if (!imuInitialized) return;
  
  unsigned long now = millis();
  if (now - lastRotationCheck < ROTATION_CHECK_INTERVAL) return;
  lastRotationCheck = now;
  
  uint8_t newRotation = detectRotation();
  if (newRotation != currentRotation) {
    applyRotation(newRotation);
  }
}
