// Display graphics and drawing functions

#ifndef DISPLAY_GRAPHICS_H
#define DISPLAY_GRAPHICS_H

#include <Arduino.h>
#include "pomodoro_config.h"

// Drawing functions
void drawSplash();
void drawGrid();
void drawColorPreview();
void drawMainFunctionality();
void drawB24Placeholder();
void drawB24LoadingSpinner();
void drawTelegramPrompt();
void drawAPPrompt();
void drawCenteredText(const char *txt, int16_t cx, int16_t cy, uint16_t color, uint8_t size);
void drawPlayIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color);
void drawPauseIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color);
void drawGearIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color);
void drawTomatoIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color);
void drawPaletteIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color);
void redrawGridCell(int row, int col, bool isSelected);

// LCD initialization
void lcd_reg_init(void);

#endif // DISPLAY_GRAPHICS_H
