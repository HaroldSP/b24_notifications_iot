// Touch input handling

#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Arduino.h>
#include "esp_lcd_touch_axs5106l.h"

// Touch data structure
extern touch_data_t touch_points;
extern bool touchPressed;
extern unsigned long touchStartTime;
extern bool longPressDetected;

// Functions
void readTouchData();
void handleTouchInput();

#endif // TOUCH_HANDLER_H
