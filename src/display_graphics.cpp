// Display graphics and drawing functions implementation

#include "display_graphics.h"
#include "pomodoro_globals.h"
#include "pomodoro_config.h"
#include "color_utils.h"
#include "bitrix24.h"
#include "FreeSansBold24pt7b.h"
#include <math.h>

// --- Low-level LCD init from Waveshare demo (unchanged) ---
void lcd_reg_init(void) {
  static const uint8_t init_operations[] = {
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x11,
    END_WRITE,
    DELAY, 120,

    BEGIN_WRITE,
    WRITE_C8_D16, 0xDF, 0x98, 0x53,
    WRITE_C8_D8, 0xB2, 0x23,

    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 4,
    0x00, 0x47, 0x00, 0x6F,

    WRITE_COMMAND_8, 0xBB,
    WRITE_BYTES, 6,
    0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0,

    WRITE_C8_D16, 0xC0, 0x44, 0xA4,
    WRITE_C8_D8, 0xC1, 0x16,

    WRITE_COMMAND_8, 0xC3,
    WRITE_BYTES, 8,
    0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77,

    WRITE_COMMAND_8, 0xC4,
    WRITE_BYTES, 12,
    0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82,

    WRITE_COMMAND_8, 0xC8,
    WRITE_BYTES, 32,
    0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,
    0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00,

    WRITE_COMMAND_8, 0xD0,
    WRITE_BYTES, 5,
    0x04, 0x06, 0x6B, 0x0F, 0x00,

    WRITE_C8_D16, 0xD7, 0x00, 0x30,
    WRITE_C8_D8, 0xE6, 0x14,
    WRITE_C8_D8, 0xDE, 0x01,

    WRITE_COMMAND_8, 0xB7,
    WRITE_BYTES, 5,
    0x03, 0x13, 0xEF, 0x35, 0x35,

    WRITE_COMMAND_8, 0xC1,
    WRITE_BYTES, 3,
    0x14, 0x15, 0xC0,

    WRITE_C8_D16, 0xC2, 0x06, 0x3A,
    WRITE_C8_D16, 0xC4, 0x72, 0x12,
    WRITE_C8_D8, 0xBE, 0x00,
    WRITE_C8_D8, 0xDE, 0x02,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3,
    0x00, 0x02, 0x00,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3,
    0x01, 0x02, 0x00,

    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x35, 0x00,
    WRITE_C8_D8, 0x3A, 0x05,

    WRITE_COMMAND_8, 0x2A,
    WRITE_BYTES, 4,
    0x00, 0x22, 0x00, 0xCD,

    WRITE_COMMAND_8, 0x2B,
    WRITE_BYTES, 4,
    0x00, 0x00, 0x01, 0x3F,

    WRITE_C8_D8, 0xDE, 0x02,

    WRITE_COMMAND_8, 0xE5,
    WRITE_BYTES, 3,
    0x00, 0x02, 0x00,

    WRITE_C8_D8, 0xDE, 0x00,
    WRITE_C8_D8, 0x36, 0x00,
    WRITE_COMMAND_8, 0x21,
    END_WRITE,

    DELAY, 10,

    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x29,
    END_WRITE
  };
  bus->batchOperation(init_operations, sizeof(init_operations));
}

// --- Helper: draw golden "R" splash (used as stopped screen) ---
void drawSplash() {
  gfx->fillScreen(COLOR_BLACK);

  // Use selected work color for logo
  uint16_t workColor = selectedWorkColor;
  
  // Check if we're in landscape mode
  bool isLandscape = (currentRotation == 1 || currentRotation == 3);
  
  int16_t centerX = gfx->width() / 2;
  int16_t centerY = gfx->height() / 2;
  int16_t radius = 70;
  int16_t borderWidth = 5;

  for (int16_t i = 0; i < borderWidth; i++) {
    gfx->drawCircle(centerX, centerY, radius - i, workColor);
  }

  gfx->setFont(&FreeSansBold24pt7b);
  gfx->setTextColor(workColor);
  gfx->setTextSize(2, 2, 0);
  gfx->setCursor(centerX - 33, centerY + 30);
  gfx->print("R");
  
  // Draw gear icon (settings button)
  int16_t gearSize = 36;
  int16_t gearCenterX, gearCenterY;
  
  if (isLandscape) {
    // Landscape: gear button on the right side
    gearCenterX = gfx->width() - 40;
    gearCenterY = gfx->height() / 2;
  } else {
    // Portrait: gear button at the bottom center
    gearCenterX = gfx->width() / 2;
    gearCenterY = gfx->height() - 40;
  }
  
  // Set gear button bounds for touch detection
  int padding = 8;
  gearBtnLeft = gearCenterX - gearSize/2 - padding;
  gearBtnRight = gearCenterX + gearSize/2 + padding;
  gearBtnTop = gearCenterY - gearSize/2 - padding;
  gearBtnBottom = gearCenterY + gearSize/2 + padding;
  
  // Draw gear icon
  drawGearIcon(gearCenterX, gearCenterY, gearSize, workColor);
  gearBtnValid = true;
  
  // Disable old work/rest buttons
  workBtnValid = false;
  restBtnValid = false;
}

// Helper function to redraw a single grid cell (for partial updates to prevent flickering)
void redrawGridCell(int row, int col, bool isSelected) {
  if (row < 0 || col < 0 || row >= gridNumRows || col >= gridNumCols) return;
  
  int colorIndex = row * gridNumCols + col;
  if (colorIndex >= paletteSize) return;
  
  int16_t cellX = gridStartX + col * gridCellWidth;
  int16_t cellY = row * gridCellHeight;
  uint16_t cellColor = paletteColors[colorIndex];
  
  // Fill the cell with color
  gfx->fillRect(cellX, cellY, gridCellWidth, gridCellHeight, cellColor);
  
  // Draw grid lines around the cell
  bool isLandscape = (currentRotation == 1 || currentRotation == 3);
  uint16_t gridColor = COLOR_BLACK;
  
  // Draw right border (if not last column)
  if (col < gridNumCols - 1) {
    gfx->drawFastVLine(cellX + gridCellWidth, cellY, gridCellHeight, gridColor);
  }
  
  // Draw bottom border (if not last row or if landscape)
  if (isLandscape || row < gridNumRows - 2) {
    gfx->drawFastHLine(cellX, cellY + gridCellHeight, gridCellWidth, gridColor);
  }
  
  // Highlight selected cell with white border
  if (isSelected) {
    // Draw thick white border (4 pixels inside)
    for (int i = 0; i < 4; i++) {
      gfx->drawRect(cellX + i, cellY + i, 
                    gridCellWidth - i * 2, gridCellHeight - i * 2, 
                    COLOR_WHITE);
    }
  }
}

// --- Helper: draw grid view (3 columns, X rows with square cells) ---
void drawGrid() {
  gfx->fillScreen(COLOR_BLACK);
  
  // Reset last selected cell when redrawing entire grid
  lastSelectedGridRow = -1;
  lastSelectedGridCol = -1;
  
  // Check if we're in landscape mode
  bool isLandscape = (currentRotation == 1 || currentRotation == 3);
  
  int16_t screenWidth = gfx->width();
  int16_t screenHeight = gfx->height();
  
  // Save grid parameters to global variables for touch detection
  // Use fixed cell size (43x43px) for both portrait and landscape modes
  gridCellWidth = 43;
  gridCellHeight = 43;
  
  if (isLandscape) {
    // Landscape: 5 columns, 4 rows
    gridNumCols = 5;
    gridNumRows = 4;
  } else {
    // Portrait: 3 columns, X rows
    gridNumCols = 3;
    gridNumRows = screenHeight / gridCellHeight;
  }
  
  // Calculate total grid width
  int16_t gridWidth = gridNumCols * gridCellWidth;
  // Center the grid horizontally and save to global
  // In landscape mode, shift grid a bit to the left
  if (isLandscape) {
    gridStartX = (screenWidth - gridWidth) / 2 - 10;  // Shift 10px to the left
  } else {
    gridStartX = (screenWidth - gridWidth) / 2;  // Centered in portrait
  }
  
  // Grid lines color (black)
  uint16_t gridColor = COLOR_BLACK;
  
  // Calculate last row Y position
  // In portrait mode, last row is for buttons, so skip it
  // In landscape mode, all rows are for colors, buttons are on the right
  int16_t lastRowY;
  int rowsForColors;
  if (isLandscape) {
    lastRowY = gridNumRows * gridCellHeight;
    rowsForColors = gridNumRows;  // All rows for colors
  } else {
    lastRowY = (gridNumRows - 1) * gridCellHeight;
    rowsForColors = gridNumRows - 1;  // Last row is for buttons
  }
  
  // Fill grid cells with palette colors and highlight selected
  int colorIndex = 0;
  for (int row = 0; row < rowsForColors; row++) {
    for (int col = 0; col < gridNumCols; col++) {
      int16_t cellX = gridStartX + col * gridCellWidth;
      int16_t cellY = row * gridCellHeight;
      
      // Use palette color
      if (colorIndex < paletteSize) {
        uint16_t cellColor = paletteColors[colorIndex];
        
        // Fill the cell with color
        gfx->fillRect(cellX, cellY, gridCellWidth, gridCellHeight, cellColor);
        
        // Highlight selected cell with white border
        if (colorIndex == tempSelectedColorIndex) {
          // Draw thick white border (3 pixels)
          for (int i = 0; i < 3; i++) {
            gfx->drawRect(cellX + i, cellY + i, 
                          gridCellWidth - i * 2, gridCellHeight - i * 2, 
                          COLOR_WHITE);
          }
        }
      }
      
      colorIndex++;
      if (colorIndex >= paletteSize) break;
    }
    if (colorIndex >= paletteSize) break;
  }
  
  // Draw grid lines (black) on top of colored cells
  for (int col = 1; col < gridNumCols; col++) {
    int16_t x = gridStartX + col * gridCellWidth;
    gfx->drawFastVLine(x, 0, lastRowY, gridColor);
  }
  
  // Draw horizontal lines (row separators)
  if (isLandscape) {
    // Landscape: draw lines between all rows
    for (int row = 1; row < gridNumRows; row++) {
      int16_t y = row * gridCellHeight;
      if (y < screenHeight) {
        gfx->drawFastHLine(gridStartX, y, gridWidth, gridColor);
      }
    }
    // Draw bottom border
    gfx->drawFastHLine(gridStartX, lastRowY, gridWidth, gridColor);
  } else {
    // Portrait: draw lines between rows (but not last one, which is for buttons)
    for (int row = 1; row < gridNumRows - 1; row++) {
      int16_t y = row * gridCellHeight;
      if (y < screenHeight) {
        gfx->drawFastHLine(gridStartX, y, gridWidth, gridColor);
      }
    }
    // Draw bottom border line
    gfx->drawFastHLine(gridStartX, lastRowY, gridWidth, gridColor);
  }
  
  // Calculate button size - try size 5 or 6 for bigger buttons
  const char *cancelTxt = "X";
  const char *confirmTxt = "V";  // Use "V" instead of "✓" for better compatibility
  
  int16_t x1, y1;
  uint16_t w1, h1, w2, h2;
  gfx->setFont(nullptr);
  uint8_t textSize = 5;  // Try size 5 for bigger buttons (can be 6 if needed)
  gfx->setTextSize(textSize, textSize, 0);
  
  // Get bounds for both texts to ensure same button size
  gfx->getTextBounds(cancelTxt, 0, 0, &x1, &y1, &w1, &h1);
  gfx->getTextBounds(confirmTxt, 0, 0, &x1, &y1, &w2, &h2);
  
  // Use maximum width and height for both buttons to make them same size
  uint16_t maxW = (w1 > w2) ? w1 : w2;
  uint16_t maxH = (h1 > h2) ? h1 : h2;
  
  // Calculate button size with padding
  int padding = 6;
  int16_t btnWidth = maxW + padding * 2;
  int16_t btnHeight = maxH + padding * 2;
  
  int16_t textOffsetX = 2;  // Move right a bit
  int16_t textOffsetY = 2;  // Move down a bit
  
  if (isLandscape) {
    // Landscape: buttons on right side, 1 column, 2 rows, V on top, X below
    // Place buttons in the right empty space (moved right a bit)
    int16_t btnX = screenWidth - 23;  // Right side with smaller margin (moved right)
    int16_t btnSpacing = 20;  // Space between V and X buttons vertically
    int16_t centerY = screenHeight / 2;
    
    // Confirm button (V) on top
    int16_t confirmCenterY = centerY - btnSpacing / 2 - btnHeight / 2;
    gridConfirmBtnLeft   = btnX - btnWidth / 2;
    gridConfirmBtnRight  = btnX + btnWidth / 2;
    gridConfirmBtnTop    = confirmCenterY - btnHeight / 2;
    gridConfirmBtnBottom = confirmCenterY + btnHeight / 2;
    
    // Draw border around confirm button (golden color)
    gfx->drawRect(gridConfirmBtnLeft, gridConfirmBtnTop,
                  btnWidth,
                  btnHeight,
                  COLOR_GOLD);
    
    // Draw "V" text centered
    drawCenteredText(confirmTxt, btnX + textOffsetX, confirmCenterY + textOffsetY, COLOR_GOLD, textSize);
    gridConfirmBtnValid = true;
    
    // Cancel button (X) below
    int16_t cancelCenterY = centerY + btnSpacing / 2 + btnHeight / 2;
    gridCancelBtnLeft   = btnX - btnWidth / 2;
    gridCancelBtnRight  = btnX + btnWidth / 2;
    gridCancelBtnTop    = cancelCenterY - btnHeight / 2;
    gridCancelBtnBottom = cancelCenterY + btnHeight / 2;
    
    // Draw border around cancel button (golden color)
    gfx->drawRect(gridCancelBtnLeft, gridCancelBtnTop,
                  btnWidth,
                  btnHeight,
                  COLOR_GOLD);
    
    // Draw "X" text centered
    drawCenteredText(cancelTxt, btnX + textOffsetX, cancelCenterY + textOffsetY, COLOR_GOLD, textSize);
    gridCancelBtnValid = true;
  } else {
    // Portrait: buttons in bottom row, X on left, V on right, centered
    int16_t bottomRowY = lastRowY;
    int16_t bottomRowHeight = gridCellHeight;
    int16_t bottomRowCenterY = bottomRowY + bottomRowHeight / 2 + 15;
    
    // Center buttons in the bottom row with space between them
    int16_t spaceBetween = 20;  // Space between buttons
    int16_t totalButtonsWidth = btnWidth * 2 + spaceBetween;
    int16_t buttonsStartX = gridStartX + (gridWidth - totalButtonsWidth) / 2;
    
    // Left button "X"
    int16_t cancelCenterX = buttonsStartX + btnWidth / 2;
    gridCancelBtnLeft   = buttonsStartX;
    gridCancelBtnRight  = buttonsStartX + btnWidth;
    gridCancelBtnTop    = bottomRowCenterY - btnHeight / 2;
    gridCancelBtnBottom = bottomRowCenterY + btnHeight / 2;
    
    // Draw border around cancel button (golden color)
    gfx->drawRect(gridCancelBtnLeft, gridCancelBtnTop,
                  btnWidth,
                  btnHeight,
                  COLOR_GOLD);
    
    // Draw "X" text centered
    drawCenteredText(cancelTxt, cancelCenterX + textOffsetX, bottomRowCenterY + textOffsetY, COLOR_GOLD, textSize);
    gridCancelBtnValid = true;
    
    // Right button "V" (checkmark)
    int16_t confirmStartX = buttonsStartX + btnWidth + spaceBetween;
    int16_t confirmCenterX = confirmStartX + btnWidth / 2;
    gridConfirmBtnLeft   = confirmStartX;
    gridConfirmBtnRight  = confirmStartX + btnWidth;
    gridConfirmBtnTop    = bottomRowCenterY - btnHeight / 2;
    gridConfirmBtnBottom = bottomRowCenterY + btnHeight / 2;
    
    // Draw border around confirm button (golden color)
    gfx->drawRect(gridConfirmBtnLeft, gridConfirmBtnTop,
                  btnWidth,
                  btnHeight,
                  COLOR_GOLD);
    
    // Draw "V" text centered
    drawCenteredText(confirmTxt, confirmCenterX + textOffsetX, bottomRowCenterY + textOffsetY, COLOR_GOLD, textSize);
    gridConfirmBtnValid = true;
  }
}

// --- Helper: centered text using getTextBounds ---
void drawCenteredText(const char *txt, int16_t cx, int16_t cy, uint16_t color, uint8_t size) {
  int16_t x1, y1;
  uint16_t w, h;
  gfx->setFont(nullptr);
  gfx->setTextSize(size, size, 0);
  gfx->getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  int16_t x = cx - (int16_t)w / 2;
  // Center text vertically: cy is the desired center, y1 is offset from baseline (usually negative)
  // cursorY + y1 + h/2 = cy, so cursorY = cy - y1 - h/2
  int16_t y = cy - y1 - (int16_t)h / 2;
  gfx->setCursor(x, y);
  gfx->setTextColor(color);
  gfx->print(txt);
}

// --- Helper: draw play icon (triangle) ---
void drawPlayIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color) {
  // Draw a filled triangle pointing right
  int16_t halfH = size / 2;
  int16_t w = size * 3 / 4;
  // Triangle vertices: left-top, left-bottom, right-center
  gfx->fillTriangle(
    cx - w/2, cy - halfH,      // top-left
    cx - w/2, cy + halfH,      // bottom-left
    cx + w/2, cy,              // right center
    color
  );
}

// --- Helper: draw pause icon (two bars) ---
void drawPauseIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color) {
  // Draw two vertical bars
  int16_t barWidth = size / 4;
  int16_t barHeight = size;
  int16_t gap = size / 4;
  // Left bar
  gfx->fillRect(cx - gap - barWidth, cy - barHeight/2, barWidth, barHeight, color);
  // Right bar  
  gfx->fillRect(cx + gap, cy - barHeight/2, barWidth, barHeight, color);
}

// --- Helper: draw gear icon (settings) - Material Design Icons cog style ---
void drawGearIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color) {
  // Material Design Icons cog: outer circle with evenly spaced teeth + inner circle with hole
  // Fill space between circles
  int16_t outerRadius = size / 2 - 1;
  int16_t innerRadius = size / 3;
  int16_t toothLength = size / 6;
  int16_t toothWidth = size / 10;  // Fixed width for all teeth
  int numTeeth = 8;  // Standard 8 teeth for MDI cog
  
  // Fill the space between outer and inner circles (gear body)
  for (int16_t r = innerRadius + 1; r < outerRadius; r++) {
    gfx->drawCircle(cx, cy, r, color);
  }
  
  // Draw outer circle
  gfx->drawCircle(cx, cy, outerRadius, color);
  
  // Draw teeth (rectangular, evenly spaced, all equal size)
  // Use pixel-by-pixel filling to ensure all teeth are exactly the same thickness
  for (int i = 0; i < numTeeth; i++) {
    float angle = (i * 2.0f * PI) / numTeeth;
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    
    // Tooth extends from outerRadius to outerRadius + toothLength
    // Calculate perpendicular direction for tooth width
    float perpAngle = angle + PI / 2;
    float perpCos = cosf(perpAngle);
    float perpSin = sinf(perpAngle);
    
    // Calculate tooth center line points
    float baseCenterX = cx + outerRadius * cosA;
    float baseCenterY = cy + outerRadius * sinA;
    float tipCenterX = cx + (outerRadius + toothLength) * cosA;
    float tipCenterY = cy + (outerRadius + toothLength) * sinA;
    
    // Calculate perpendicular offset (half width)
    float halfWidth = toothWidth / 2.0f;
    
    // Fill tooth pixel by pixel along its length to ensure consistent thickness
    int steps = (int)(toothLength * 2);  // More steps for smoother filling
    for (int step = 0; step <= steps; step++) {
      float t = (float)step / steps;
      float centerX = baseCenterX + t * (tipCenterX - baseCenterX);
      float centerY = baseCenterY + t * (tipCenterY - baseCenterY);
      
      // Draw a line perpendicular to the tooth direction at this point
      int16_t x1 = (int16_t)(centerX + halfWidth * perpCos);
      int16_t y1 = (int16_t)(centerY + halfWidth * perpSin);
      int16_t x2 = (int16_t)(centerX - halfWidth * perpCos);
      int16_t y2 = (int16_t)(centerY - halfWidth * perpSin);
      
      // Draw line to fill the tooth width at this point
      gfx->drawLine(x1, y1, x2, y2, color);
    }
  }
  
  // Draw inner circle (hollow center)
  gfx->fillCircle(cx, cy, innerRadius, COLOR_BLACK);
  gfx->drawCircle(cx, cy, innerRadius, color);
  
  // Draw center hole (smaller circle inside)
  int16_t holeRadius = innerRadius / 2;
  gfx->fillCircle(cx, cy, holeRadius, COLOR_BLACK);
}

// --- Helper: draw color preview screen ---
void drawColorPreview() {
  gfx->fillScreen(COLOR_BLACK);
  
  uint16_t workColor = tempPreviewColor;
  // Use tempPreviewRestColor if set, otherwise use inverted work color
  uint16_t restColor = (tempPreviewRestColor != 0) ? tempPreviewRestColor : invertColor(tempPreviewColor);
  
  // Check if we're in landscape mode
  bool isLandscape = (currentRotation == 1 || currentRotation == 3);
  
  int16_t centerX = gfx->width() / 2;
  int16_t centerY = gfx->height() / 2;
  
  int16_t swatchWidth = 80;
  int16_t swatchHeight = 40;
  
  if (isLandscape) {
    // Landscape: work on left, rest on right
    int16_t workX = centerX - 60;
    int16_t restX = centerX + 60;
    
    // Draw "WORK" label and color swatch on left (clickable)
    drawCenteredText("WORK", workX, centerY - 40, workColor, 2);
    previewWorkSwatchLeft = workX - swatchWidth/2;
    previewWorkSwatchRight = workX + swatchWidth/2;
    previewWorkSwatchTop = centerY - swatchHeight/2;
    previewWorkSwatchBottom = centerY + swatchHeight/2;
    gfx->fillRect(previewWorkSwatchLeft, previewWorkSwatchTop, swatchWidth, swatchHeight, workColor);
    gfx->drawRect(previewWorkSwatchLeft, previewWorkSwatchTop, swatchWidth, swatchHeight, COLOR_WHITE);
    previewWorkSwatchValid = true;
    
    // Draw "REST" label and color swatch on right (clickable)
    drawCenteredText("REST", restX, centerY - 40, restColor, 2);
    previewRestSwatchLeft = restX - swatchWidth/2;
    previewRestSwatchRight = restX + swatchWidth/2;
    previewRestSwatchTop = centerY - swatchHeight/2;
    previewRestSwatchBottom = centerY + swatchHeight/2;
    gfx->fillRect(previewRestSwatchLeft, previewRestSwatchTop, swatchWidth, swatchHeight, restColor);
    gfx->drawRect(previewRestSwatchLeft, previewRestSwatchTop, swatchWidth, swatchHeight, COLOR_WHITE);
    previewRestSwatchValid = true;
  } else {
    // Portrait: work at top, rest at bottom
    int16_t workY = centerY - 60;
    int16_t restY = centerY + 60;
    
    // Draw "WORK" label and color swatch at top (clickable)
    drawCenteredText("WORK", centerX, workY - 30, workColor, 2);
    previewWorkSwatchLeft = centerX - swatchWidth/2;
    previewWorkSwatchRight = centerX + swatchWidth/2;
    previewWorkSwatchTop = workY - swatchHeight/2;
    previewWorkSwatchBottom = workY + swatchHeight/2;
    gfx->fillRect(previewWorkSwatchLeft, previewWorkSwatchTop, swatchWidth, swatchHeight, workColor);
    gfx->drawRect(previewWorkSwatchLeft, previewWorkSwatchTop, swatchWidth, swatchHeight, COLOR_WHITE);
    previewWorkSwatchValid = true;
    
    // Draw "REST" label and color swatch at bottom (clickable)
    drawCenteredText("REST", centerX, restY - 30, restColor, 2);
    previewRestSwatchLeft = centerX - swatchWidth/2;
    previewRestSwatchRight = centerX + swatchWidth/2;
    previewRestSwatchTop = restY - swatchHeight/2;
    previewRestSwatchBottom = restY + swatchHeight/2;
    gfx->fillRect(previewRestSwatchLeft, previewRestSwatchTop, swatchWidth, swatchHeight, restColor);
    gfx->drawRect(previewRestSwatchLeft, previewRestSwatchTop, swatchWidth, swatchHeight, COLOR_WHITE);
    previewRestSwatchValid = true;
  }
  
  // Draw X (cancel) and V (confirm) buttons
  int16_t btnSize = 30;
  int padding = 6;
  
  if (isLandscape) {
    // Landscape: buttons on right side, 1 column, 2 rows, V on top, X below
    // Moved right a bit to avoid overlapping other UI
    int16_t btnX = gfx->width() - 30;  // Right side with smaller margin (moved right)
    int16_t confirmCenterY = centerY - 30;  // V on top
    int16_t cancelCenterY = centerY + 30;    // X below
    
    // Confirm button (V) on top
    previewConfirmBtnLeft = btnX - btnSize/2 - padding;
    previewConfirmBtnRight = btnX + btnSize/2 + padding;
    previewConfirmBtnTop = confirmCenterY - btnSize/2 - padding;
    previewConfirmBtnBottom = confirmCenterY + btnSize/2 + padding;
    gfx->drawRect(previewConfirmBtnLeft, previewConfirmBtnTop,
                  previewConfirmBtnRight - previewConfirmBtnLeft,
                  previewConfirmBtnBottom - previewConfirmBtnTop,
                  COLOR_WHITE);
    drawCenteredText("V", btnX, confirmCenterY, COLOR_WHITE, 3);
    previewConfirmBtnValid = true;
    
    // Cancel button (X) below
    previewCancelBtnLeft = btnX - btnSize/2 - padding;
    previewCancelBtnRight = btnX + btnSize/2 + padding;
    previewCancelBtnTop = cancelCenterY - btnSize/2 - padding;
    previewCancelBtnBottom = cancelCenterY + btnSize/2 + padding;
    gfx->drawRect(previewCancelBtnLeft, previewCancelBtnTop,
                  previewCancelBtnRight - previewCancelBtnLeft,
                  previewCancelBtnBottom - previewCancelBtnTop,
                  COLOR_WHITE);
    drawCenteredText("X", btnX, cancelCenterY, COLOR_WHITE, 3);
    previewCancelBtnValid = true;
  } else {
    // Portrait: buttons at bottom, X on left, V on right
    int16_t btnY = gfx->height() - 40;
    
    // Cancel button (X) on left
    int16_t cancelCenterX = gfx->width() / 4;
    previewCancelBtnLeft = cancelCenterX - btnSize/2 - padding;
    previewCancelBtnRight = cancelCenterX + btnSize/2 + padding;
    previewCancelBtnTop = btnY - btnSize/2 - padding;
    previewCancelBtnBottom = btnY + btnSize/2 + padding;
    gfx->drawRect(previewCancelBtnLeft, previewCancelBtnTop,
                  previewCancelBtnRight - previewCancelBtnLeft,
                  previewCancelBtnBottom - previewCancelBtnTop,
                  COLOR_WHITE);
    drawCenteredText("X", cancelCenterX, btnY, COLOR_WHITE, 3);
    previewCancelBtnValid = true;
    
    // Confirm button (V) on right
    int16_t confirmCenterX = gfx->width() * 3 / 4;
    previewConfirmBtnLeft = confirmCenterX - btnSize/2 - padding;
    previewConfirmBtnRight = confirmCenterX + btnSize/2 + padding;
    previewConfirmBtnTop = btnY - btnSize/2 - padding;
    previewConfirmBtnBottom = btnY + btnSize/2 + padding;
    gfx->drawRect(previewConfirmBtnLeft, previewConfirmBtnTop,
                  previewConfirmBtnRight - previewConfirmBtnLeft,
                  previewConfirmBtnBottom - previewConfirmBtnTop,
                  COLOR_WHITE);
    drawCenteredText("V", confirmCenterX, btnY, COLOR_WHITE, 3);
    previewConfirmBtnValid = true;
  }
}

// --- Helper: draw tomato icon (pomodoro) ---
void drawTomatoIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color) {
  // Draw a simple tomato shape: circle with a small stem on top
  int16_t radius = size / 2 - 2;
  
  // Draw main tomato body (circle) - always red
  gfx->fillCircle(cx, cy, radius, COLOR_RED);
  gfx->drawCircle(cx, cy, radius, COLOR_BLACK);
  
  // Draw stem on top (small rectangle)
  int16_t stemWidth = size / 6;
  int16_t stemHeight = size / 4;
  gfx->fillRect(cx - stemWidth/2, cy - radius - stemHeight/2, stemWidth, stemHeight, COLOR_GREEN);
  gfx->drawRect(cx - stemWidth/2, cy - radius - stemHeight/2, stemWidth, stemHeight, COLOR_BLACK);
  
  // Draw a small highlight circle (top-left)
  int16_t highlightRadius = radius / 3;
  gfx->fillCircle(cx - radius/3, cy - radius/3, highlightRadius, COLOR_WHITE);
}

// --- Helper: draw palette icon (mdi mdi-palette style) ---
void drawPaletteIcon(int16_t cx, int16_t cy, int16_t size, uint16_t color) {
  // Draw palette shape: rounded rectangle with thumb hole on left side
  int16_t width = size;
  int16_t height = size * 3 / 4;
  int16_t cornerRadius = size / 8;
  int16_t thumbHoleRadius = size / 6;
  
  // Draw main palette body (rounded rectangle) - always brown
  gfx->fillRoundRect(cx - width/2, cy - height/2, width, height, cornerRadius, COLOR_COFFEE);
  gfx->drawRoundRect(cx - width/2, cy - height/2, width, height, cornerRadius, COLOR_BLACK);
  
  // Draw thumb hole (circle on left side, shifted right a bit)
  int16_t thumbX = cx - width/2 + thumbHoleRadius + size/12;  // Moved right
  int16_t thumbY = cy;
  gfx->fillCircle(thumbX, thumbY, thumbHoleRadius, COLOR_BLACK);
  
  // Draw small color dots on palette (representing paint)
  // Position dots to avoid thumb hole and edges
  int16_t dotRadius = size / 14;  // Slightly smaller dots
  int16_t dotSpacing = size / 5;   // Closer spacing between dots
  int16_t thumbAreaRight = thumbX + thumbHoleRadius + dotRadius + 2; // Right edge of thumb hole area
  int16_t startX = cx + size/20;  // Moved right (positive offset)
  int16_t startY = cy - height/6;  // Moved down (less negative offset)
  
  // Draw 4 color dots in a 2x2 grid, avoiding thumb hole area
  if (startX - dotSpacing/2 > thumbAreaRight) {
    // Top row
    gfx->fillCircle(startX - dotSpacing/2, startY, dotRadius, COLOR_RED);
    gfx->fillCircle(startX + dotSpacing/2, startY, dotRadius, COLOR_BLUE);
    // Bottom row
    gfx->fillCircle(startX - dotSpacing/2, startY + dotSpacing, dotRadius, COLOR_GREEN);
    gfx->fillCircle(startX + dotSpacing/2, startY + dotSpacing, dotRadius, COLOR_YELLOW);
  } else {
    // If thumb hole takes too much space, use single row shifted right
    startX = cx + size/15;  // Shift right (positive offset)
    gfx->fillCircle(startX, startY, dotRadius, COLOR_RED);
    gfx->fillCircle(startX + dotSpacing, startY, dotRadius, COLOR_BLUE);
    gfx->fillCircle(startX, startY + dotSpacing, dotRadius, COLOR_GREEN);
    gfx->fillCircle(startX + dotSpacing, startY + dotSpacing, dotRadius, COLOR_YELLOW);
  }
}

// --- Helper: draw main functionality screen ---
void drawMainFunctionality() {
  gfx->fillScreen(COLOR_BLACK);
  
  bool isLandscape = (currentRotation == 1 || currentRotation == 3);
  int16_t screenWidth = gfx->width();
  int16_t screenHeight = gfx->height();
  int16_t centerX = screenWidth / 2;
  int16_t centerY = screenHeight / 2;
  
  uint16_t btnColor = selectedWorkColor; // Use selected work color for buttons
  int16_t btnSize = 60; // Button icon size
  int16_t btnPadding = 10; // Padding around button for touch area
  int16_t btnSpacing = 30; // Space between buttons
  
  if (isLandscape) {
    // Landscape: buttons arranged horizontally
    int16_t totalWidth = btnSize * 3 + btnSpacing * 2;
    int16_t startX = centerX - totalWidth / 2;
    
    // B24 button (left)
    int16_t b24X = startX + btnSize / 2;
    int16_t b24Y = centerY;
    mainMenuB24BtnLeft = b24X - btnSize/2 - btnPadding;
    mainMenuB24BtnRight = b24X + btnSize/2 + btnPadding;
    mainMenuB24BtnTop = b24Y - btnSize/2 - btnPadding;
    mainMenuB24BtnBottom = b24Y + btnSize/2 + btnPadding;
    gfx->drawRect(mainMenuB24BtnLeft, mainMenuB24BtnTop, 
                  mainMenuB24BtnRight - mainMenuB24BtnLeft,
                  mainMenuB24BtnBottom - mainMenuB24BtnTop,
                  btnColor);
    drawCenteredText("B24", b24X, b24Y, btnColor, 3);
    mainMenuB24BtnValid = true;
    
    // Tomato button (center)
    int16_t tomatoX = startX + btnSize + btnSpacing + btnSize / 2;
    int16_t tomatoY = centerY;
    mainMenuTomatoBtnLeft = tomatoX - btnSize/2 - btnPadding;
    mainMenuTomatoBtnRight = tomatoX + btnSize/2 + btnPadding;
    mainMenuTomatoBtnTop = tomatoY - btnSize/2 - btnPadding;
    mainMenuTomatoBtnBottom = tomatoY + btnSize/2 + btnPadding;
    gfx->drawRect(mainMenuTomatoBtnLeft, mainMenuTomatoBtnTop,
                  mainMenuTomatoBtnRight - mainMenuTomatoBtnLeft,
                  mainMenuTomatoBtnBottom - mainMenuTomatoBtnTop,
                  btnColor);
    drawTomatoIcon(tomatoX, tomatoY, btnSize, btnColor);
    mainMenuTomatoBtnValid = true;
    
    // Palette button (right)
    int16_t paletteX = startX + btnSize * 2 + btnSpacing * 2 + btnSize / 2;
    int16_t paletteY = centerY;
    mainMenuPaletteBtnLeft = paletteX - btnSize/2 - btnPadding;
    mainMenuPaletteBtnRight = paletteX + btnSize/2 + btnPadding;
    mainMenuPaletteBtnTop = paletteY - btnSize/2 - btnPadding;
    mainMenuPaletteBtnBottom = paletteY + btnSize/2 + btnPadding;
    gfx->drawRect(mainMenuPaletteBtnLeft, mainMenuPaletteBtnTop,
                  mainMenuPaletteBtnRight - mainMenuPaletteBtnLeft,
                  mainMenuPaletteBtnBottom - mainMenuPaletteBtnTop,
                  btnColor);
    drawPaletteIcon(paletteX, paletteY, btnSize, btnColor);
    mainMenuPaletteBtnValid = true;
  } else {
    // Portrait: buttons arranged vertically
    int16_t totalHeight = btnSize * 3 + btnSpacing * 2;
    int16_t startY = centerY - totalHeight / 2;
    
    // B24 button (top)
    int16_t b24X = centerX;
    int16_t b24Y = startY + btnSize / 2;
    mainMenuB24BtnLeft = b24X - btnSize/2 - btnPadding;
    mainMenuB24BtnRight = b24X + btnSize/2 + btnPadding;
    mainMenuB24BtnTop = b24Y - btnSize/2 - btnPadding;
    mainMenuB24BtnBottom = b24Y + btnSize/2 + btnPadding;
    gfx->drawRect(mainMenuB24BtnLeft, mainMenuB24BtnTop,
                  mainMenuB24BtnRight - mainMenuB24BtnLeft,
                  mainMenuB24BtnBottom - mainMenuB24BtnTop,
                  btnColor);
    drawCenteredText("B24", b24X, b24Y, btnColor, 3);
    mainMenuB24BtnValid = true;
    
    // Tomato button (center)
    int16_t tomatoX = centerX;
    int16_t tomatoY = startY + btnSize + btnSpacing + btnSize / 2;
    mainMenuTomatoBtnLeft = tomatoX - btnSize/2 - btnPadding;
    mainMenuTomatoBtnRight = tomatoX + btnSize/2 + btnPadding;
    mainMenuTomatoBtnTop = tomatoY - btnSize/2 - btnPadding;
    mainMenuTomatoBtnBottom = tomatoY + btnSize/2 + btnPadding;
    gfx->drawRect(mainMenuTomatoBtnLeft, mainMenuTomatoBtnTop,
                  mainMenuTomatoBtnRight - mainMenuTomatoBtnLeft,
                  mainMenuTomatoBtnBottom - mainMenuTomatoBtnTop,
                  btnColor);
    drawTomatoIcon(tomatoX, tomatoY, btnSize, btnColor);
    mainMenuTomatoBtnValid = true;
    
    // Palette button (bottom)
    int16_t paletteX = centerX;
    int16_t paletteY = startY + btnSize * 2 + btnSpacing * 2 + btnSize / 2;
    mainMenuPaletteBtnLeft = paletteX - btnSize/2 - btnPadding;
    mainMenuPaletteBtnRight = paletteX + btnSize/2 + btnPadding;
    mainMenuPaletteBtnTop = paletteY - btnSize/2 - btnPadding;
    mainMenuPaletteBtnBottom = paletteY + btnSize/2 + btnPadding;
    gfx->drawRect(mainMenuPaletteBtnLeft, mainMenuPaletteBtnTop,
                  mainMenuPaletteBtnRight - mainMenuPaletteBtnLeft,
                  mainMenuPaletteBtnBottom - mainMenuPaletteBtnTop,
                  btnColor);
    drawPaletteIcon(paletteX, paletteY, btnSize, btnColor);
    mainMenuPaletteBtnValid = true;
  }
}

// --- Helper: draw B24 placeholder screen ---
void drawB24Placeholder() {
  gfx->fillScreen(COLOR_BLACK);
  
  int16_t screenWidth = gfx->width();
  int16_t screenHeight = gfx->height();
  bool isLandscape = (currentRotation == 1 || currentRotation == 3);
  
  // Header
  int16_t headerHeight = 30;
  gfx->fillRect(0, 0, screenWidth, headerHeight, COLOR_DARK_BLUE);
  drawCenteredText("Bitrix24", screenWidth / 2, headerHeight / 2, COLOR_WHITE, 2);
  
  int16_t contentStartY = headerHeight;
  int16_t contentHeight = screenHeight - headerHeight;
  
  // Helper function to draw a section (just circle with number, no text)
  // If totalUnreadCount > 0, label2 is ignored and subtitle shows "All unread msg: [totalUnreadCount]" with number in red
  auto drawSection = [&](int16_t x, int16_t y, int16_t w, int16_t h, 
                         const char* label1, const char* label2, const char* badgeText, uint16_t totalUnreadCount = 0) {
    int16_t centerX = x + w / 2;
    int16_t centerY = y + h / 2;
    
    // Draw border
    gfx->drawRect(x, y, w, h, COLOR_GRAY);
    
    // Draw title at top of cell
    // ADJUSTABLE PARAMETER for title position:
    int16_t titleTopOffset = 12;  // Distance from top border to title (increase = move down)
    
    int16_t titleY = y + titleTopOffset;
    drawCenteredText(label1, centerX, titleY, COLOR_WHITE, 1);
    
    // Calculate badge size - use most of available space with padding
    // ADJUSTABLE PARAMETERS for circle size calculation:
    int16_t padding = 10;                    // Padding around circle (decrease = bigger circle)
    float landscapeWidthPercent = 0.75;      // Landscape: max % of cell width (increase = bigger)
    float portraitHeightPercent = 0.5;       // Portrait: max % of cell height (increase = bigger)
    
    int16_t badgeSize;
    
    if (isLandscape) {
      // HORIZONTAL MODE CALCULATION:
      // Step 1: Try to use full height minus padding
      badgeSize = h - padding * 2;
      // Step 2: Limit to percentage of width (prevents circle from being too wide)
      // Formula: badgeSize = min(h - padding*2, w * landscapeWidthPercent)
      if (badgeSize > w * landscapeWidthPercent) {
        badgeSize = w * landscapeWidthPercent;
      }
    } else {
      // VERTICAL MODE CALCULATION:
      // Step 1: Try to use full width minus padding
      badgeSize = w - padding * 2;
      // Step 2: Limit to percentage of height (prevents circle from being too tall)
      // Formula: badgeSize = min(w - padding*2, h * portraitHeightPercent)
      if (badgeSize > h * portraitHeightPercent) {
        badgeSize = h * portraitHeightPercent;
      }
    }
    
    int16_t badgeRadius = badgeSize / 2;
    
    // Circle is perfectly centered in the cell
    int16_t badgeX = centerX;
    int16_t badgeY = centerY;
    
    // Draw circle centered
    gfx->fillCircle(badgeX, badgeY, badgeRadius, selectedWorkColor);
    gfx->drawCircle(badgeX, badgeY, badgeRadius, COLOR_WHITE);
    
    // Draw text centered in circle - use proper text centering
    // ADJUSTABLE PARAMETERS for text position inside circle:
    int16_t textOffsetX = 2;  // Move text right (increase) or left (decrease)
    int16_t textOffsetY = 2;  // Move text down (increase) or up (decrease)
    
    // Calculate text size - start with base size and scale down if needed
    uint8_t textSize = (badgeSize > 50) ? 4 : 3;
    
    // Get text bounds to check if text fits in circle
    int16_t x1, y1;
    uint16_t textW, textH;
    gfx->setFont(nullptr);
    gfx->setTextSize(textSize, textSize, 0);
    gfx->getTextBounds(badgeText, 0, 0, &x1, &y1, &textW, &textH);
    
    // Scale down text size if it doesn't fit in circle (with some margin)
    // Allow text to use ~80% of circle diameter (radius * 1.6 = 0.8 * diameter)
    int16_t maxTextWidth = badgeRadius * 1.6;
    int16_t maxTextHeight = badgeRadius * 1.6;
    while ((textW > maxTextWidth || textH > maxTextHeight) && textSize > 1) {
      textSize--;
      gfx->setTextSize(textSize, textSize, 0);
      gfx->getTextBounds(badgeText, 0, 0, &x1, &y1, &textW, &textH);
    }
    
    // Calculate text position: center of circle minus half text dimensions + offset
    int16_t textX = badgeX - textW / 2 + textOffsetX;
    // For vertical centering: badgeY is center, y1 is baseline offset (usually negative)
    // We want: cursorY + y1 + textH/2 = badgeY
    // So: cursorY = badgeY - y1 - textH/2
    int16_t textY = badgeY - y1 - textH / 2 + textOffsetY;
    
    gfx->setCursor(textX, textY);
    gfx->setTextColor(COLOR_WHITE);
    gfx->print(badgeText);
    
    // Draw subtitle at bottom of cell
    // ADJUSTABLE PARAMETER for subtitle position:
    int16_t subtitleBottomOffset = 12;  // Distance from bottom border to subtitle (increase = move up)
    
    int16_t subtitleY = y + h - subtitleBottomOffset;
    if (totalUnreadCount > 0) {
      // Special case: show "All unread msg: [NUMBER]" with number in red
      char subtitleText[32];
      snprintf(subtitleText, sizeof(subtitleText), "All msgs: ");
      
      // Draw "All msgs: " in gray - use same vertical centering as drawCenteredText
      int16_t x1, y1;
      uint16_t textW, textH;
      gfx->setFont(nullptr);
      gfx->setTextSize(1, 1, 0);
      gfx->getTextBounds(subtitleText, 0, 0, &x1, &y1, &textW, &textH);
      
      // ADJUSTABLE PARAMETER for subtitle vertical position:
      // Adjust subtitleY to move text up (decrease) or down (increase)
      // Formula matches drawCenteredText: cursorY = targetY - y1 - h/2
      int16_t subtitleTextY = subtitleY - y1 - (int16_t)textH / 2;
      
      int16_t textX = centerX - textW / 2;
      gfx->setCursor(textX, subtitleTextY);
      gfx->setTextColor(COLOR_GRAY);
      gfx->print(subtitleText);
      
      // Draw number in red - get bounds for number text too
      char numberText[16];
      snprintf(numberText, sizeof(numberText), "%u", totalUnreadCount);
      int16_t numX1, numY1;
      uint16_t numW, numH;
      gfx->getTextBounds(numberText, 0, 0, &numX1, &numY1, &numW, &numH);
      int16_t numberTextY = subtitleY - numY1 - (int16_t)numH / 2;
      
      gfx->setCursor(textX + textW, numberTextY);
      gfx->setTextColor(COLOR_RED);
      gfx->print(numberText);
    } else if (label2) {
      drawCenteredText(label2, centerX, subtitleY, COLOR_GRAY, 1);
    }
  };
  
  // Get Bitrix24 counts
  Bitrix24Counts counts = getBitrix24Counts();
  
  // Convert counts to strings
  char msgCount[16];
  char taskCount[16];
  char groupCount[16];
  
  if (counts.valid) {
    snprintf(msgCount, sizeof(msgCount), "%u", counts.unreadMessages);
    snprintf(taskCount, sizeof(taskCount), "%u", counts.undoneTasks);
    snprintf(groupCount, sizeof(groupCount), "%u", counts.groupsProjects);
  } else {
    // Use "0" if data not available yet
    strcpy(msgCount, "0");
    strcpy(taskCount, "0");
    strcpy(groupCount, "0");
  }
  
  if (isLandscape) {
    // Landscape: 3 columns, 1 row
    int16_t sectionWidth = screenWidth / 3;
    int16_t sectionHeight = contentHeight;
    
    // Section 1: Unread Messages (show total unread in subtitle)
    drawSection(0, contentStartY, sectionWidth, sectionHeight, 
                "Unread Messages", "(Dialogs)", msgCount, counts.totalUnreadMessages);
    
    // Section 2: Undone Tasks
    drawSection(sectionWidth, contentStartY, sectionWidth, sectionHeight,
                "Undone Tasks", "Auto & BP", taskCount);
    
    // Section 3: Groups & Projects
    drawSection(sectionWidth * 2, contentStartY, sectionWidth, sectionHeight,
                "Groups & Projects", "(All)", groupCount);
    
    // Draw vertical dividers
    gfx->drawFastVLine(sectionWidth, contentStartY, sectionHeight, COLOR_GRAY);
    gfx->drawFastVLine(sectionWidth * 2, contentStartY, sectionHeight, COLOR_GRAY);
  } else {
    // Portrait: 1 column, 3 rows
    int16_t sectionWidth = screenWidth;
    int16_t sectionHeight = contentHeight / 3;
    
    // Section 1: Unread Messages (show total unread in subtitle)
    drawSection(0, contentStartY, sectionWidth, sectionHeight,
                "Unread Messages", "(Dialogs)", msgCount, counts.totalUnreadMessages);
    
    // Section 2: Undone Tasks
    drawSection(0, contentStartY + sectionHeight, sectionWidth, sectionHeight,
                "Undone Tasks", "Auto & BP", taskCount);
    
    // Section 3: Groups & Projects
    drawSection(0, contentStartY + sectionHeight * 2, sectionWidth, sectionHeight,
                "Groups & Projects", "(All)", groupCount);
    
    // Draw horizontal dividers
    gfx->drawFastHLine(0, contentStartY + sectionHeight, screenWidth, COLOR_GRAY);
    gfx->drawFastHLine(0, contentStartY + sectionHeight * 2, screenWidth, COLOR_GRAY);
  }
}