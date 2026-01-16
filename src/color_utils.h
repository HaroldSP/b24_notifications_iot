// Color utilities and palette

#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

#include <Arduino.h>
#include "pomodoro_config.h"

// Function to invert a 16-bit RGB565 color
uint16_t invertColor(uint16_t color);

// Global palette array for color picker
extern const uint16_t paletteColors[];
extern const int paletteSize;

#endif // COLOR_UTILS_H
