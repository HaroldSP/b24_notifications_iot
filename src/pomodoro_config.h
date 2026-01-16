// Configuration constants and pin definitions

#ifndef POMODORO_CONFIG_H
#define POMODORO_CONFIG_H

#include <Arduino.h>

// Backlight pin (official: GPIO23 = LCD_BL)
#define GFX_BL 23

// Rotation (0 = portrait, like official demo)
#define ROTATION 0

// Touch pins from official Waveshare LVGL example
#define TP_SDA 18
#define TP_SCL 19
#define TP_RST 20
#define TP_INT 21

// IMU (QMI8658) for auto-rotation
#define IMU_ADDRESS 0x6B  // QMI8658 default I2C address

// Timer durations
const unsigned long POMODORO_DURATION_1 = 1UL * 60UL * 1000UL;   // 1 minute
const unsigned long POMODORO_DURATION_25 = 25UL * 60UL * 1000UL;  // 25 minutes
const unsigned long POMODORO_DURATION_50 = 50UL * 60UL * 1000UL; // 50 minutes
const unsigned long REST_DURATION_1 = 1UL * 60UL * 1000UL;        // 1 minute
const unsigned long REST_DURATION_5 = 5UL * 60UL * 1000UL;        // 5 minutes
const unsigned long REST_DURATION_10 = 10UL * 60UL * 1000UL;      // 10 minutes

// Timing constants
const unsigned long FLASH_DURATION = 500;                  // ms
const unsigned long LONG_PRESS_MS = 1000;                 // long press
const unsigned long SHORT_TAP_BLOCK_MS = 1500;  // Block short taps for 1.5s after timer start
const unsigned long TP_INT_DEBOUNCE_MS = 200;  // Ignore brief HIGH pulses
const unsigned long TAP_INDICATOR_DURATION = 500;  // ms
const unsigned long ROTATION_CHECK_INTERVAL = 2000;  // Check every 2 seconds
const float ROTATION_THRESHOLD = 0.5;  // Threshold in g for rotation detection

// Touch padding
const int16_t TOUCH_PADDING = 15;  // 15px extra on each side
const int TAP_RADIUS = 4;  // Tap indicator radius

// Color definitions
const uint16_t COLOR_BLACK = 0x0000;
const uint16_t COLOR_GOLD  = 0xFCE0; // tuned golden
const uint16_t COLOR_BLUE  = 0x001F;
const uint16_t COLOR_GREEN = 0x07E0;
const uint16_t COLOR_RED   = 0xF800;
const uint16_t COLOR_COFFEE = 0x8200; // Brown/coffee color
const uint16_t COLOR_CYAN   = 0x07FF;
const uint16_t COLOR_MAGENTA = 0xF81F;
const uint16_t COLOR_YELLOW = 0xFFE0;
const uint16_t COLOR_ORANGE = 0xFBF9; // Pink (rgb(255,192,203))
const uint16_t COLOR_PURPLE = 0x780F; // Purple
const uint16_t COLOR_PINK = 0xFC1F;   // Pink
const uint16_t COLOR_TEAL = 0x07EF;   // Teal
const uint16_t COLOR_LIME = 0x87E0;   // Lime green
const uint16_t COLOR_INDIGO = 0x4810; // Indigo
const uint16_t COLOR_CORAL = 0xFA00;  // International Orange (#ff4f00, rgb(255,79,0))
const uint16_t COLOR_LAVENDER = 0x9C1F; // Lavender
const uint16_t COLOR_EMERALD = 0x07D0; // Emerald green
const uint16_t COLOR_WHITE = 0xFFFF;  // White
const uint16_t COLOR_DARK_BLUE = 0x000F; // Dark blue
const uint16_t COLOR_OLIVE = 0x8400;   // Olive green
const uint16_t COLOR_TURQUOISE = 0x04FF; // Turquoise (more contrast than teal)
const uint16_t COLOR_VIOLET = 0x901A;  // Violet (more contrast than purple)
const uint16_t COLOR_SALMON = 0xFC60;  // Salmon
const uint16_t COLOR_MINT = 0x87FF;    // Mint green (lighter than cyan)
const uint16_t COLOR_NAVY = 0x0010;    // Navy blue
const uint16_t COLOR_GRAY = 0x8410;    // Gray

#endif // POMODORO_CONFIG_H
