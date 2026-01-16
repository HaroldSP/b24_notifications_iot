// Display update logic

#ifndef DISPLAY_UPDATES_H
#define DISPLAY_UPDATES_H

#include <Arduino.h>

// Display update functions
void updateDisplay();
void drawTimer();
void drawProgressCircle(float progress, int centerX, int centerY, int radius, uint16_t color);
void displayStoppedState();

#endif // DISPLAY_UPDATES_H
