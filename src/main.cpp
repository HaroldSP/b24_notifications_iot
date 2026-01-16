// Arduino_GFX-based Pomodoro timer for Waveshare ESP32-C6-LCD-1.47
// Refactored version - main file only includes modules

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <math.h>
#include <Preferences.h>
#include <FastIMU.h>
#include "FreeSansBold24pt7b.h"
#include "esp_lcd_touch_axs5106l.h"

// Module includes
#include "pomodoro_types.h"
#include "pomodoro_config.h"
#include "pomodoro_globals.h"
#include "wifi_telegram.h"
#include "color_utils.h"
#include "storage.h"
#include "display_graphics.h"
#include "timer_logic.h"
#include "touch_handler.h"
#include "display_updates.h"
#include "auto_rotation.h"

// --- Arduino setup / loop ---
void setup(void) {
  Serial.begin(115200);
  Serial.println("Pomodoro Timer (Arduino_GFX) starting...");

  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }

  lcd_reg_init();
  gfx->setRotation(ROTATION);
  gfx->fillScreen(COLOR_BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
  Serial.println("Initializing I2C for touch...");
#endif

  // Init I2C for touch
  Serial.println("Initializing touch controller...");
  Wire.begin(TP_SDA, TP_SCL);
  delay(100);
  Serial.print("TP_INT pin state after init: ");
  Serial.println(digitalRead(TP_INT));
  Serial.println("Touch init complete. Ready for input.");
  
  // Init touch driver
  bsp_touch_init(&Wire, TP_RST, TP_INT, gfx->getRotation(), gfx->width(), gfx->height());
  pinMode(TP_INT, INPUT_PULLUP);

  // Initialize IMU (QMI8658) for auto-rotation
  // IMU shares I2C bus with touch controller
  Serial.println("Initializing IMU (QMI8658)...");
  int imuErr = imu.init(imuCalibration, IMU_ADDRESS);
  if (imuErr != 0) {
    Serial.print("IMU init failed with error: ");
    Serial.println(imuErr);
    imuInitialized = false;
  } else {
    Serial.println("IMU initialized successfully!");
    imuInitialized = true;
  }

  // Load saved color from NVS
  loadSelectedColor();
  
  // Connect to WiFi
  connectWiFi();
  
  // Initialize Telegram bot
  initTelegramBot();
  
  // Start Telegram task on separate core
  startTelegramTask();

  displayStoppedState();
}

void loop() {
  // Handle touch FIRST - highest priority for responsiveness
  handleTouchInput();
  
  // Process commands from Telegram (non-blocking - just checks flags)
  processTelegramCommands();
  
  updateTimer();
  updateDisplay();
  checkAutoRotation();  // Check IMU for auto-rotation

  // Tap indicator disabled for better touch responsiveness
  // (was causing lag due to drawing overhead)

  delay(2);  // Reduced delay for faster loop
}
