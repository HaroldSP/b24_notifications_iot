// Touch input handling implementation

#include "touch_handler.h"
#include "pomodoro_globals.h"
#include "pomodoro_config.h"
#include "display_graphics.h"
#include "display_updates.h"
#include "timer_logic.h"
#include "storage.h"
#include "color_utils.h"
#include "wifi_telegram.h"
#include "bitrix24.h"
#include <Wire.h>
#include <string.h>

// Touch detection variables
touch_data_t touch_points;
bool touchPressed = false;
unsigned long touchStartTime = 0;
bool longPressDetected = false;

// Static variables for touch reading
static bool lastIntState = HIGH;
static unsigned long lastTpIntLowTime = 0;

// Read touch data directly from I2C (working method from test)
void readTouchData() {
  bool currentIntState = digitalRead(TP_INT);
  
  if (currentIntState == LOW && lastIntState == HIGH) {
    // Touch just started - read immediately
    delayMicroseconds(100);
    Wire.beginTransmission(0x63);
    Wire.write(0x01);
    if (Wire.endTransmission() == 0) {
      uint8_t bytesRead = Wire.requestFrom(0x63, 14);
      if (bytesRead >= 14) {
        uint8_t data[14];
        Wire.readBytes(data, 14);
        uint8_t touch_num = data[1];
        if (touch_num > 0 && touch_num <= 5) {
          touch_points.touch_num = touch_num;
          for (uint8_t i = 0; i < touch_num; i++) {
            touch_points.coords[i].x = ((uint16_t)(data[2+i*6] & 0x0f)) << 8;
            touch_points.coords[i].x |= data[3+i*6];
            touch_points.coords[i].y = (((uint16_t)(data[4+i*6] & 0x0f)) << 8);
            touch_points.coords[i].y |= data[5+i*6];
            
            uint16_t x = touch_points.coords[i].x;
            uint16_t y = touch_points.coords[i].y;
            // Native touch panel is 172x320 (portrait)
            // Transform to rotated display coordinates (using working transformation from test)
            switch (gfx->getRotation()) {
              case 0:  // Portrait normal
                touch_points.coords[i].x = gfx->width() - 1 - x;
                touch_points.coords[i].y = y;
                break;
              case 1:  // Landscape right (320x172)
                touch_points.coords[i].y = x;
                touch_points.coords[i].x = y;
                break;
              case 2:  // Portrait upside down
                touch_points.coords[i].x = x;
                touch_points.coords[i].y = gfx->height() - 1 - y;
                break;
              case 3:  // Landscape left (320x172)
                touch_points.coords[i].y = gfx->height() - 1 - x;
                touch_points.coords[i].x = gfx->width() - 1 - y;
                break;
            }
          }
        } else {
          touch_points.touch_num = 0;
        }
      }
    }
  } else if (currentIntState == HIGH && lastIntState == HIGH) {
    // Only reset if we're sure touch is released (both current and last are HIGH)
    touch_points.touch_num = 0;
  } else if (currentIntState == LOW && lastIntState == LOW) {
    // Touch still active - read again for continuous tracking
    Wire.beginTransmission(0x63);
    Wire.write(0x01);
    if (Wire.endTransmission() == 0) {
      uint8_t bytesRead = Wire.requestFrom(0x63, 14);
      if (bytesRead >= 14) {
        uint8_t data[14];
        Wire.readBytes(data, 14);
        uint8_t touch_num = data[1];
        if (touch_num > 0 && touch_num <= 5) {
          touch_points.touch_num = touch_num;
          for (uint8_t i = 0; i < touch_num; i++) {
            touch_points.coords[i].x = ((uint16_t)(data[2+i*6] & 0x0f)) << 8;
            touch_points.coords[i].x |= data[3+i*6];
            touch_points.coords[i].y = (((uint16_t)(data[4+i*6] & 0x0f)) << 8);
            touch_points.coords[i].y |= data[5+i*6];
            
            uint16_t x = touch_points.coords[i].x;
            uint16_t y = touch_points.coords[i].y;
            // Native touch panel is 172x320 (portrait)
            // Transform to rotated display coordinates (using working transformation from test)
            switch (gfx->getRotation()) {
              case 0:  // Portrait normal
                touch_points.coords[i].x = gfx->width() - 1 - x;
                touch_points.coords[i].y = y;
                break;
              case 1:  // Landscape right (320x172)
                touch_points.coords[i].y = x;
                touch_points.coords[i].x = y;
                break;
              case 2:  // Portrait upside down
                touch_points.coords[i].x = x;
                touch_points.coords[i].y = gfx->height() - 1 - y;
                break;
              case 3:  // Landscape left (320x172)
                touch_points.coords[i].y = gfx->height() - 1 - x;
                touch_points.coords[i].x = gfx->width() - 1 - y;
                break;
            }
          }
          // touch_num is 0 in data, but TP_INT is still LOW - don't reset yet
          // Keep previous touch_num value to maintain touch state
        }
      } else {
        // I2C read failed, but TP_INT is still LOW - keep touch state
      }
    }
  }
  
  lastIntState = currentIntState;
}

void handleTouchInput() {
  // Read TP_INT with debouncing to filter out noise
  bool tpIntLow = (digitalRead(TP_INT) == LOW);
  static unsigned long lastTpIntHighTime = 0;
  
  if (tpIntLow) {
    lastTpIntLowTime = millis();
    // Reset HIGH time when we see LOW again
    lastTpIntHighTime = 0;
  } else {
    // TP_INT is HIGH - track when it went HIGH
    if (lastTpIntHighTime == 0) {
      lastTpIntHighTime = millis();
    }
  }
  
  // Read touch data for coordinates
  readTouchData();

  // Consider touch active if TP_INT is LOW (original logic with debouncing)
  // Use debounce logic to handle noise but still detect touches
  bool currentlyTouched = tpIntLow || 
                          (millis() - lastTpIntLowTime < TP_INT_DEBOUNCE_MS);
  
  // Capture last valid touch position whenever we have touch data
  if (touch_points.touch_num > 0) {
    lastTouchX = touch_points.coords[0].x;
    lastTouchY = touch_points.coords[0].y;
    lastTouchValid = true;
  }

  if (currentlyTouched && !touchPressed) {
    Serial.println(">>> TOUCH PRESSED <<<");
    touchPressed = true;
    touchStartTime = millis();
    longPressDetected = false;
    // (optional) could capture initial touch position here if needed later
  } else if (!currentlyTouched && touchPressed) {
    unsigned long touchDuration = millis() - touchStartTime;
    Serial.print(">>> TOUCH RELEASED after ");
    Serial.print(touchDuration);
    Serial.println(" ms <<<");

    // Only process short tap if long press wasn't already handled
    // Reduced threshold from 50ms to 10ms for faster response
    // Also block short taps for a short period after timer start to prevent accidental pause
    unsigned long timeSinceStart = (timerStartTime > 0) ? (millis() - timerStartTime) : SHORT_TAP_BLOCK_MS + 1;
    bool blockShortTap = (timeSinceStart < SHORT_TAP_BLOCK_MS);
    
    if (!longPressDetected && touchDuration > 10 && !blockShortTap) {
      // Base position for this tap (use last valid touch, as TP_INT may already be HIGH)
      int16_t tx = lastTouchValid ? lastTouchX : -1;
      int16_t ty = lastTouchValid ? lastTouchY : -1;

      // Always draw tap indicator if we have a valid position
      if (lastTouchValid && tx >= 0 && ty >= 0) {
        tapIndicatorX = tx;
        tapIndicatorY = ty;
        tapIndicatorActive = true;
        tapIndicatorStart = millis();
      }

      // Check for grid view buttons (X and ✓) when grid is active
      bool inGridCancelButton = false;
      bool inGridConfirmButton = false;
      int8_t tappedColorIndex = -1;  // Color cell tapped in grid (-1 = none)
      
      // Check for color preview buttons - declare here so they're visible in the entire if block
      bool inPreviewCancelButton = false;
      bool inPreviewConfirmButton = false;
      bool inPreviewRestSwatch = false;
      bool inPreviewWorkSwatch = false;
      
      if (gridViewActive && lastTouchValid && tx >= 0 && ty >= 0) {
        // Check if we're in landscape mode for proper touch detection
        bool isLandscape = (currentRotation == 1 || currentRotation == 3);
        
        // First check if tap is on a color cell
        // In landscape mode, all rows are for colors
        // In portrait mode, last row is for buttons
        int16_t maxRowY;
        if (isLandscape) {
          maxRowY = gridNumRows * gridCellHeight;  // All rows for colors
        } else {
          maxRowY = (gridNumRows - 1) * gridCellHeight;  // Last row is for buttons
        }
        
        if (ty < maxRowY) {
          // Calculate which cell was tapped
          int col = (tx - gridStartX) / gridCellWidth;
          int row = ty / gridCellHeight;
          if (col >= 0 && col < gridNumCols && row >= 0 && row < gridNumRows &&
              tx >= gridStartX && tx < gridStartX + gridNumCols * gridCellWidth) {
            int colorIdx = row * gridNumCols + col;
            if (colorIdx >= 0 && colorIdx < paletteSize) {
              tappedColorIndex = colorIdx;
            }
          }
        }
        
        // Check cancel button - with extra touch padding
        if (gridCancelBtnValid) {
          if (tx >= gridCancelBtnLeft - TOUCH_PADDING && tx <= gridCancelBtnRight + TOUCH_PADDING &&
              ty >= gridCancelBtnTop - TOUCH_PADDING && ty <= gridCancelBtnBottom + TOUCH_PADDING) {
            inGridCancelButton = true;
          }
        }
        // Check confirm button - with extra touch padding
        if (gridConfirmBtnValid) {
          if (tx >= gridConfirmBtnLeft - TOUCH_PADDING && tx <= gridConfirmBtnRight + TOUCH_PADDING &&
              ty >= gridConfirmBtnTop - TOUCH_PADDING && ty <= gridConfirmBtnBottom + TOUCH_PADDING) {
            inGridConfirmButton = true;
          }
        }
      }

      // Check for main functionality screen buttons
      bool inMainMenuB24Btn = false;
      bool inMainMenuTomatoBtn = false;
      bool inMainMenuPaletteBtn = false;
      bool inMainMenuAPBtn = false;
      if (currentViewMode == VIEW_MODE_MAIN_MENU && lastTouchValid && tx >= 0 && ty >= 0) {
        if (mainMenuB24BtnValid) {
          if (tx >= mainMenuB24BtnLeft - TOUCH_PADDING && tx <= mainMenuB24BtnRight + TOUCH_PADDING &&
              ty >= mainMenuB24BtnTop - TOUCH_PADDING && ty <= mainMenuB24BtnBottom + TOUCH_PADDING) {
            inMainMenuB24Btn = true;
          }
        }
        if (mainMenuTomatoBtnValid) {
          if (tx >= mainMenuTomatoBtnLeft - TOUCH_PADDING && tx <= mainMenuTomatoBtnRight + TOUCH_PADDING &&
              ty >= mainMenuTomatoBtnTop - TOUCH_PADDING && ty <= mainMenuTomatoBtnBottom + TOUCH_PADDING) {
            inMainMenuTomatoBtn = true;
          }
        }
        if (mainMenuPaletteBtnValid) {
          if (tx >= mainMenuPaletteBtnLeft - TOUCH_PADDING && tx <= mainMenuPaletteBtnRight + TOUCH_PADDING &&
              ty >= mainMenuPaletteBtnTop - TOUCH_PADDING && ty <= mainMenuPaletteBtnBottom + TOUCH_PADDING) {
            inMainMenuPaletteBtn = true;
          }
        }
        if (mainMenuAPBtnValid) {
          if (tx >= mainMenuAPBtnLeft - TOUCH_PADDING && tx <= mainMenuAPBtnRight + TOUCH_PADDING &&
              ty >= mainMenuAPBtnTop - TOUCH_PADDING && ty <= mainMenuAPBtnBottom + TOUCH_PADDING) {
            inMainMenuAPBtn = true;
          }
        }
      }
      
      // Check for home screen gear button (settings) when stopped - with extra touch padding
      bool inGearButton = false;
      if (currentState == STOPPED && currentViewMode == 0 && lastTouchValid && tx >= 0 && ty >= 0) {
        if (gearBtnValid) {
          if (tx >= gearBtnLeft - TOUCH_PADDING && tx <= gearBtnRight + TOUCH_PADDING &&
              ty >= gearBtnTop - TOUCH_PADDING && ty <= gearBtnBottom + TOUCH_PADDING) {
            inGearButton = true;
          }
        }
      }
      
      // Check for color preview buttons - with extra touch padding
      if (currentViewMode == VIEW_MODE_PREVIEW && lastTouchValid && tx >= 0 && ty >= 0) {
        if (previewCancelBtnValid) {
          if (tx >= previewCancelBtnLeft - TOUCH_PADDING && tx <= previewCancelBtnRight + TOUCH_PADDING &&
              ty >= previewCancelBtnTop - TOUCH_PADDING && ty <= previewCancelBtnBottom + TOUCH_PADDING) {
            inPreviewCancelButton = true;
          }
        }
        if (previewConfirmBtnValid) {
          if (tx >= previewConfirmBtnLeft - TOUCH_PADDING && tx <= previewConfirmBtnRight + TOUCH_PADDING &&
              ty >= previewConfirmBtnTop - TOUCH_PADDING && ty <= previewConfirmBtnBottom + TOUCH_PADDING) {
            inPreviewConfirmButton = true;
          }
        }
        // Check work color swatch - with extra touch padding
        if (previewWorkSwatchValid) {
          if (tx >= previewWorkSwatchLeft - TOUCH_PADDING && tx <= previewWorkSwatchRight + TOUCH_PADDING &&
              ty >= previewWorkSwatchTop - TOUCH_PADDING && ty <= previewWorkSwatchBottom + TOUCH_PADDING) {
            inPreviewWorkSwatch = true;
          }
        }
        // Check for rest color swatch click
        if (previewRestSwatchValid) {
          if (tx >= previewRestSwatchLeft - TOUCH_PADDING && tx <= previewRestSwatchRight + TOUCH_PADDING &&
              ty >= previewRestSwatchTop - TOUCH_PADDING && ty <= previewRestSwatchBottom + TOUCH_PADDING) {
            inPreviewRestSwatch = true;
          }
        }
      }

      // Check for mode button click first (with extra touch padding)
      bool inModeButton = false;
      if (modeBtnValid && lastTouchValid && tx >= 0 && ty >= 0) {
        if (tx >= modeBtnLeft - TOUCH_PADDING && tx <= modeBtnRight + TOUCH_PADDING &&
            ty >= modeBtnTop - TOUCH_PADDING && ty <= modeBtnBottom + TOUCH_PADDING) {
          inModeButton = true;
        }
      }
      
      // Check for status button click (with extra touch padding)
      bool inStatusButton = false;
      if (statusBtnValid && lastTouchValid && tx >= 0 && ty >= 0) {
        if (tx >= statusBtnLeft - TOUCH_PADDING && tx <= statusBtnRight + TOUCH_PADDING &&
            ty >= statusBtnTop - TOUCH_PADDING && ty <= statusBtnBottom + TOUCH_PADDING) {
          inStatusButton = true;
        }
      }
      
      // Check for tap inside the timer circle (to toggle MM:SS <-> MM display)
      bool inCircle = false;
      if (lastTouchValid && tx >= 0 && ty >= 0 && (currentState == RUNNING || currentState == PAUSED)) {
        int16_t centerX = gfx->width() / 2;
        int16_t centerY = gfx->height() / 2;
        int16_t radius = 70;
        int16_t dx = tx - centerX;
        int16_t dy = ty - centerY;
        int16_t distSquared = dx * dx + dy * dy;
        if (distSquared <= radius * radius) {
          inCircle = true;
        }
      }

      if (inGridCancelButton) {
        // X button clicked in grid view - return to home screen without saving
        Serial.println("*** GRID CANCEL (X) BUTTON CLICKED ***");
        tempSelectedColorIndex = -1;  // Clear temporary selection
        gridViewActive = false;
        currentViewMode = VIEW_MODE_PREVIEW;  // Return to color preview
        drawColorPreview();
        displayStoppedState();  // Return to home screen
      } else if (inGridConfirmButton) {
        // ✓ button clicked in grid view - go to color preview or save rest color
        Serial.println("*** GRID CONFIRM (✓) BUTTON CLICKED ***");
        if (tempSelectedColorIndex >= 0 && tempSelectedColorIndex < paletteSize) {
          if (selectingRestColor) {
            // Saving rest color selection
            tempPreviewRestColor = paletteColors[tempSelectedColorIndex];
            Serial.print("-> Selected rest color index: ");
            Serial.print(tempSelectedColorIndex);
            Serial.print(", color: 0x");
            Serial.println(tempPreviewRestColor, HEX);
            selectingRestColor = false;
            gridViewActive = false;
            currentViewMode = VIEW_MODE_PREVIEW;  // Switch to color preview
            drawColorPreview();
          } else {
            // Saving work color selection
            tempPreviewColor = paletteColors[tempSelectedColorIndex];
            Serial.print("-> Preview color index: ");
            Serial.print(tempSelectedColorIndex);
            Serial.print(", color: 0x");
            Serial.println(tempPreviewColor, HEX);
            tempSelectedColorIndex = -1;  // Clear temporary selection
            gridViewActive = false;
            currentViewMode = VIEW_MODE_PREVIEW;  // Switch to color preview
            drawColorPreview();
          }
        }
      } else if (tappedColorIndex >= 0) {
        // Color cell tapped - select it
        Serial.print("*** COLOR CELL TAPPED: ");
        Serial.print(tappedColorIndex);
        Serial.print(" (0x");
        Serial.print(paletteColors[tappedColorIndex], HEX);
        Serial.println(") ***");
        
        // Calculate row and col from color index
        bool isLandscape = (currentRotation == 1 || currentRotation == 3);
        int16_t rowsForColors = isLandscape ? gridNumRows : (gridNumRows - 1);
        int newRow = tappedColorIndex / gridNumCols;
        int newCol = tappedColorIndex % gridNumCols;
        
        // Redraw previous selection (remove border) if it exists
        if (lastSelectedGridRow >= 0 && lastSelectedGridCol >= 0 && 
            lastSelectedGridRow < rowsForColors && lastSelectedGridCol < gridNumCols) {
          redrawGridCell(lastSelectedGridRow, lastSelectedGridCol, false);
        }
        
        // Update selection
        tempSelectedColorIndex = tappedColorIndex;
        lastSelectedGridRow = newRow;
        lastSelectedGridCol = newCol;
        
        // Redraw new selection (add border)
        if (newRow < rowsForColors && newCol < gridNumCols) {
          redrawGridCell(newRow, newCol, true);
        }
      } else if (inPreviewCancelButton) {
        // X button clicked on color preview - return to main menu without saving
        Serial.println("*** PREVIEW CANCEL (X) BUTTON CLICKED ***");
        selectingRestColor = false;
        tempPreviewRestColor = 0;
        currentViewMode = VIEW_MODE_MAIN_MENU;
        drawMainFunctionality();
      } else if (inPreviewWorkSwatch) {
        // Work color swatch clicked - open color picker for work color
        Serial.println("*** WORK COLOR SWATCH CLICKED ***");
        selectingRestColor = false;  // Selecting work color
        tempSelectedColorIndex = -1;  // Reset temporary selection
        gridViewActive = true;
        currentViewMode = VIEW_MODE_GRID;  // Grid/palette view
        drawGrid();
      } else if (inPreviewRestSwatch) {
        // Rest color swatch clicked - open color picker for rest color
        Serial.println("*** REST COLOR SWATCH CLICKED ***");
        selectingRestColor = true;
        tempSelectedColorIndex = -1;  // Reset temporary selection
        tempPreviewRestColor = 0;     // Reset to use inverted work color by default
        gridViewActive = true;
        currentViewMode = VIEW_MODE_GRID;  // Grid/palette view
        drawGrid();
      } else if (inPreviewConfirmButton) {
        // V button clicked on color preview - save colors and return to home
        Serial.println("*** PREVIEW CONFIRM (V) BUTTON CLICKED ***");
        selectedWorkColor = tempPreviewColor;
        if (tempPreviewRestColor != 0) {
          selectedRestColor = tempPreviewRestColor;
        } else {
          selectedRestColor = 0;  // Use inverted work color
        }
        saveSelectedColor();  // Save to NVS for persistence
        Serial.print("-> Saved work color: 0x");
        Serial.println(selectedWorkColor, HEX);
        if (selectedRestColor != 0) {
          Serial.print("-> Saved rest color: 0x");
          Serial.println(selectedRestColor, HEX);
        } else {
          Serial.println("-> Rest color: inverted work color");
        }
        selectingRestColor = false;
        currentViewMode = VIEW_MODE_MAIN_MENU;
        drawMainFunctionality();
      } else if (currentViewMode == VIEW_MODE_B24 && lastTouchValid && tx >= 0 && ty >= 0) {
        // On B24 screen:
        // - Tap 3rd section -> show "Open TG bot" screen for 2 seconds + send TG prompt
        // - Tap elsewhere -> return to main menu
        const int16_t headerHeight = 30;
        int16_t screenWidth = gfx->width();
        int16_t screenHeight = gfx->height();
        bool isLandscape = (currentRotation == 1 || currentRotation == 3);
        int16_t contentStartY = headerHeight;
        int16_t contentHeight = screenHeight - headerHeight;

        bool inThirdSection = false;
        if (isLandscape) {
          int16_t sectionWidth = screenWidth / 3;
          int16_t sectionHeight = contentHeight;
          int16_t x0 = sectionWidth * 2;
          int16_t y0 = contentStartY;
          inThirdSection = (tx >= x0 && tx < x0 + sectionWidth && ty >= y0 && ty < y0 + sectionHeight);
        } else {
          int16_t sectionWidth = screenWidth;
          int16_t sectionHeight = contentHeight / 3;
          int16_t x0 = 0;
          int16_t y0 = contentStartY + sectionHeight * 2;
          inThirdSection = (tx >= x0 && tx < x0 + sectionWidth && ty >= y0 && ty < y0 + sectionHeight);
        }

        if (inThirdSection) {
          Serial.println("*** B24 3RD SECTION TAPPED - OPEN TG PROMPT ***");
          // Show temporary screen for ~2 seconds
          currentViewMode = VIEW_MODE_TG_PROMPT;
          tgPromptUntilMs = millis() + 2000UL;
          drawTelegramPrompt();

          // Kick off TG configuration flow
          if (getBitrixSelectedGroupId() == 0) {
            sendTelegramMessage(
              "To choose a group/project for the counter, send its ID.\n"
              "Example: 253"
            );
          } else {
            String msg = "Selected group: ";
            msg += String(getBitrixSelectedGroupId());
            msg += "\nReply <b>all</b> to switch back to all delayed by me mode.\n";
            msg += "Or send another group ID.";
            sendTelegramMessage(msg);
          }
        } else {
          Serial.println("*** B24 SCREEN TAPPED - RETURNING TO MAIN MENU ***");
          currentViewMode = VIEW_MODE_MAIN_MENU;
          drawMainFunctionality();
        }
      } else if (inMainMenuB24Btn) {
        // B24 button clicked - open B24 placeholder screen
        Serial.println("*** MAIN MENU B24 BUTTON CLICKED ***");
        currentViewMode = VIEW_MODE_B24;
        drawB24Placeholder();
      } else if (inMainMenuTomatoBtn) {
        // Tomato button clicked - return to home/timer screen and start timer
        Serial.println("*** MAIN MENU TOMATO BUTTON CLICKED ***");
        currentViewMode = VIEW_MODE_HOME;
        if (currentState == STOPPED) {
          startTimer();
        } else {
          displayStoppedState();
        }
      } else if (inMainMenuPaletteBtn) {
        // Palette button clicked - open color preview screen
        Serial.println("*** MAIN MENU PALETTE BUTTON CLICKED ***");
        selectingRestColor = false;  // Start with work color selection
        tempSelectedColorIndex = -1;  // Reset temporary selection
        tempPreviewColor = selectedWorkColor;  // Initialize preview with current work color
        tempPreviewRestColor = (selectedRestColor != 0) ? selectedRestColor : 0;  // Initialize preview with current rest color
        currentViewMode = VIEW_MODE_PREVIEW;  // Color preview screen
        drawColorPreview();
      } else if (inMainMenuAPBtn) {
        // AP button clicked - toggle AP status
        Serial.println("*** MAIN MENU AP BUTTON CLICKED ***");
        apEnabled = !apEnabled;
        Serial.print("-> AP toggled to: ");
        Serial.println(apEnabled ? "ON" : "OFF");
        // Redraw main menu to update AP button text
        drawMainFunctionality();
      } else if (inGearButton) {
        // Gear button clicked on home screen - show main functionality screen
        Serial.println("*** GEAR BUTTON CLICKED ***");
        currentViewMode = VIEW_MODE_MAIN_MENU;  // Main functionality screen
        drawMainFunctionality();
      } else if (inModeButton) {
        // Cycle through modes: 1/1 -> 25/5 -> 50/10 -> 1/1
        Serial.println("*** MODE BUTTON CLICKED ***");
        PomodoroMode oldMode = currentMode;
        switch (currentMode) {
          case MODE_1_1:
            currentMode = MODE_25_5;
            Serial.println("-> Switched to 25/5 mode");
            break;
          case MODE_25_5:
            currentMode = MODE_50_10;
            Serial.println("-> Switched to 50/10 mode");
            break;
          case MODE_50_10:
            currentMode = MODE_1_1;
            Serial.println("-> Switched to 1/1 mode");
            break;
        }
        // Force immediate mode button update
        lastDisplayedMode = oldMode;
        updateDisplay();
      } else if (inCircle) {
        // Toggle time display mode (MM:SS <-> MM)
        Serial.println("*** CIRCLE TAPPED - TOGGLE TIME DISPLAY MODE ***");
        showMinutesOnly = !showMinutesOnly;
        Serial.print("-> Switched to ");
        Serial.println(showMinutesOnly ? "MM only" : "MM:SS");
        // Force immediate time display update
        lastShowMinutesOnly = !showMinutesOnly;  // Force redraw
        strcpy(lastTimeStr, "");  // Clear last time string to force redraw
        updateDisplay();
      } else if (inStatusButton && (currentState == RUNNING || currentState == PAUSED)) {
        Serial.println("*** STATUS BUTTON CLICKED ***");
        // Save old state before changing
        TimerState oldState = currentState;
        if (currentState == RUNNING) {
          pauseTimer();
        } else { // PAUSED
          resumeTimer();
        }
        // Force immediate button update by setting lastDisplayedState to old state
        // This ensures updateDisplay() will detect the change and redraw the button
        lastDisplayedState = oldState;
        // Force immediate display update
        updateDisplay();
      } else {
        // Tap outside button area — только индикатор
        Serial.println("*** SHORT TAP ignored (outside button) ***");
      }
    } else if (longPressDetected) {
      Serial.println("*** LONG PRESS was already handled ***");
    } else if (blockShortTap) {
      Serial.println("*** SHORT TAP blocked (too soon after timer start) ***");
    }
    touchPressed = false;
    longPressDetected = false;

  } else if (touchPressed) {
    unsigned long elapsed = millis() - touchStartTime;
    
    // Check for long press (only once per touch)
    if (elapsed > LONG_PRESS_MS && !longPressDetected) {
      longPressDetected = true;
      Serial.print("*** LONG PRESS detected! (");
      Serial.print(elapsed);
      Serial.println(" ms) ***");
      
      // Long press behavior:
      // - If timer is running/paused: stop timer and go to home
      // - If on other screens: go to home
      if (currentState == RUNNING || currentState == PAUSED) {
        Serial.println("-> Stopping timer and returning to home");
        stopTimer();
        currentViewMode = VIEW_MODE_HOME;
        displayStoppedState();
      } else if (currentViewMode != VIEW_MODE_HOME) {
        Serial.println("-> Returning to home menu");
        currentViewMode = VIEW_MODE_HOME;
        displayStoppedState();
      }
      // Don't reset - only one long press per touch
    }
  }
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    Serial.print(digitalRead(TP_INT));
    Serial.print(touch_points.touch_num);
    Serial.println(touchPressed);
    lastDebug = millis();
  }
}
