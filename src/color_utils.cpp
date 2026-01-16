// Color utilities implementation

#include "color_utils.h"

// Global palette array for color picker
const uint16_t paletteColors[] = {
  COLOR_RED,         // 0 - Red
  COLOR_ORANGE,      // 1 - Orange/Pink
  COLOR_CORAL,       // 2 - International Orange
  COLOR_YELLOW,      // 3 - Yellow
  COLOR_LIME,        // 4 - Lime green
  COLOR_GREEN,       // 5 - Green
  COLOR_MINT,        // 6 - Mint green
  COLOR_CYAN,        // 7 - Cyan
  COLOR_TURQUOISE,   // 8 - Turquoise
  COLOR_BLUE,        // 9 - Blue
  COLOR_DARK_BLUE,   // 10 - Dark blue
  COLOR_NAVY,        // 11 - Navy blue
  COLOR_INDIGO,      // 12 - Indigo
  COLOR_VIOLET,      // 13 - Violet
  COLOR_PURPLE,      // 14 - Purple
  COLOR_MAGENTA,     // 15 - Magenta
  COLOR_GOLD,        // 16 - Golden
  COLOR_COFFEE,      // 17 - Coffee/Brown
  COLOR_WHITE,       // 18 - White (added for 5x4 grid)
  0x0400             // 19 - Dark green (RGB565: 0x0400)
};
const int paletteSize = sizeof(paletteColors) / sizeof(paletteColors[0]);

// Function to invert a 16-bit RGB565 color
uint16_t invertColor(uint16_t color) {
  // RGB565: RRRRR GGGGGG BBBBB
  uint16_t r = (color >> 11) & 0x1F;  // 5 bits red
  uint16_t g = (color >> 5) & 0x3F;   // 6 bits green
  uint16_t b = color & 0x1F;          // 5 bits blue
  // Invert each channel
  r = 0x1F - r;
  g = 0x3F - g;
  b = 0x1F - b;
  return (r << 11) | (g << 5) | b;
}
