// Persistent storage (NVS) for settings

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include "pomodoro_config.h"

// Save selected color to NVS (persistent storage)
void saveSelectedColor();

// Load selected color from NVS (persistent storage)
void loadSelectedColor();

#endif // STORAGE_H
