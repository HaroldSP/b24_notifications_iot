// Display update logic implementation

#include "display_updates.h"
#include "pomodoro_globals.h"
#include "pomodoro_config.h"
#include "display_graphics.h"
#include "timer_logic.h"
#include "color_utils.h"
#include <string.h>
#include <math.h>

void updateDisplay() {
  // Temporary TG prompt screen: auto-return after timeout
  if (currentViewMode == VIEW_MODE_TG_PROMPT) {
    if (tgPromptUntilMs != 0 && millis() > tgPromptUntilMs) {
      tgPromptUntilMs = 0;
      currentViewMode = VIEW_MODE_B24;
      drawB24Placeholder();
    }
    return;
  }

  if (currentState == STOPPED) {
    // Check view mode: 0 = home, 1 = grid/palette, 2 = color preview, 3 = main menu, 4 = B24
    if (currentViewMode == VIEW_MODE_HOME && !gridViewActive) {
      // Normal stopped state is handled by displayStoppedState()
      return;
    }
    if (currentViewMode == VIEW_MODE_GRID || gridViewActive) {
      // Grid view is active, no need to update (grid is static)
      return;
    }
    if (currentViewMode == VIEW_MODE_PREVIEW) {
      // Color preview is active, no need to update (static)
      return;
    }
    if (currentViewMode == VIEW_MODE_MAIN_MENU) {
      // Main menu is active, no need to update (static)
      return;
    }
    if (currentViewMode == VIEW_MODE_B24) {
      // B24 screen is active, no need to update (static for now)
      return;
    }
    return;
  } else {
    // Update display exactly once per second for smooth timer
    static unsigned long lastDisplayUpdate = 0;
    unsigned long now = millis();
    
    // Update every 1000ms (1 second) exactly
    if (now - lastDisplayUpdate >= 1000) {
      drawTimer();
      lastDisplayUpdate = now;  // Use current time, not lastDisplayUpdate + 1000, to prevent drift
    }
  }
}

void drawTimer() {
  unsigned long elapsed = 0;
  if (currentState == RUNNING) {
    elapsed = millis() - startTime;
  } else if (currentState == PAUSED) {
    elapsed = elapsedBeforePause;
  }

  unsigned long duration = getCurrentDuration();
  unsigned long remaining = (elapsed >= duration) ? 0 : (duration - elapsed);
  unsigned long minutes = remaining / 60000UL;
  unsigned long seconds = (remaining % 60000UL) / 1000UL;

  // Format time string based on display mode
  char timeStr[10];
  if (showMinutesOnly) {
    sprintf(timeStr, "%02lu", minutes);  // MM only
  } else {
    sprintf(timeStr, "%02lu:%02lu", minutes, seconds);  // MM:SS
  }

  float progress = (float)elapsed / (float)duration;
  if (progress < 0) progress = 0;
  if (progress > 1) progress = 1;
  
  int centerX = gfx->width() / 2;
  int centerY = gfx->height() / 2;
  int radius = 70;
  
  // Check if we're in landscape mode (rotation 1 or 3)
  bool isLandscape = (currentRotation == 1 || currentRotation == 3);
  
  // Get current UI color based on work/rest session
  uint16_t uiColor = getCurrentUIColor();
  
  // Only redraw everything on first call or if state changed
  if (!displayInitialized) {
    gfx->fillScreen(COLOR_BLACK);
    drawProgressCircle(progress, centerX, centerY, radius, uiColor);
    displayInitialized = true;
    
    const char *statusTxt = nullptr;
    uint16_t statusColor = uiColor;
    bool useIcon = false;  // Use icon for pause/start
    bool isPauseIcon = false;
    
    // Status button: when running -> pause icon, when paused -> play icon
    if (currentState == PAUSED) {
      useIcon = true;
      isPauseIcon = false;  // Play icon
    } else if (currentState == RUNNING) {
      useIcon = true;
      isPauseIcon = true;   // Pause icon
    } else if (isWorkSession) {
      statusTxt = "work";
    } else {
      statusTxt = "rest";
    }
    
    // Calculate button size
    int16_t x1, y1;
    uint16_t w, h;
    int16_t iconSize = 24;  // Icon size
    
    if (useIcon) {
      w = iconSize + 8;
      h = iconSize;
    } else {
      gfx->setFont(nullptr);
      gfx->setTextSize(3, 3, 0);
      gfx->getTextBounds(statusTxt, 0, 0, &x1, &y1, &w, &h);
    }

    int padding = 6;
    int16_t statusCenterX, statusCenterY;
    
    if (isLandscape) {
      // Landscape: status button on the right side, vertically centered
      statusCenterX = gfx->width() - 35;
      statusCenterY = gfx->height() / 2;
      statusBtnLeft   = statusCenterX - (int16_t)w / 2 - padding;
      statusBtnRight  = statusCenterX + (int16_t)w / 2 + padding;
      statusBtnTop    = statusCenterY - (int16_t)h / 2 - padding;
      statusBtnBottom = statusCenterY + (int16_t)h / 2 + padding;
    } else {
      // Portrait: status button at the bottom center
      statusCenterX = gfx->width() / 2;
      statusCenterY = gfx->height() - 30;
      statusBtnLeft   = statusCenterX - (int16_t)w / 2 - padding;
      statusBtnRight  = statusCenterX + (int16_t)w / 2 + padding;
      statusBtnTop    = statusCenterY - (int16_t)h / 2 - padding;
      statusBtnBottom = statusCenterY + (int16_t)h / 2 + padding;
    }

    // Draw 1-pixel border around button
    gfx->drawRect(statusBtnLeft, statusBtnTop,
                  statusBtnRight - statusBtnLeft,
                  statusBtnBottom - statusBtnTop,
                  statusColor);

    // Draw icon or text centered inside the button
    int16_t btnCenterY = (statusBtnTop + statusBtnBottom) / 2;
    int16_t btnCenterX = (statusBtnLeft + statusBtnRight) / 2;
    if (useIcon) {
      if (isPauseIcon) {
        drawPauseIcon(btnCenterX, btnCenterY, iconSize, statusColor);
      } else {
        drawPlayIcon(btnCenterX, btnCenterY, iconSize, statusColor);
      }
    } else {
      drawCenteredText(statusTxt, btnCenterX, btnCenterY, statusColor, 3);
    }
    statusBtnValid = true;
    lastDisplayedState = currentState;  // Initialize state tracking
    
    // Draw mode button (left side in landscape, top center in portrait)
    // Single "M" button for mode selection - same size as pause/resume button
    const char *modeTxt = "M";
    
    gfx->setFont(nullptr);
    gfx->setTextSize(3, 3, 0);  // Same text size as status button
    
    // Use same size as status button (iconSize = 24, padding = 6)
    int16_t modeIconSize = 24;  // Same as status button icon size
    int16_t modeW = modeIconSize + 8;  // Same width calculation as status button (32)
    int16_t modeH = modeIconSize;  // Same height as status button (24)
    padding = 6;  // Same padding as status button
    int16_t modeCenterX, modeCenterY;
    
    if (isLandscape) {
      // Landscape: mode button on the left side, vertically centered
      modeCenterX = 35;
      modeCenterY = gfx->height() / 2;
      modeBtnLeft   = modeCenterX - (int16_t)modeW / 2 - padding;
      modeBtnRight  = modeCenterX + (int16_t)modeW / 2 + padding;
      modeBtnTop    = modeCenterY - (int16_t)modeH / 2 - padding;
      modeBtnBottom = modeCenterY + (int16_t)modeH / 2 + padding;
    } else {
      // Portrait: mode button at the top center
      int16_t topMargin = 24;
      modeCenterX = gfx->width() / 2;
      modeCenterY = topMargin + (int16_t)modeH / 2 + padding;
      modeBtnLeft   = modeCenterX - (int16_t)modeW / 2 - padding;
      modeBtnRight  = modeCenterX + (int16_t)modeW / 2 + padding;
      modeBtnTop    = topMargin;
      modeBtnBottom = modeCenterY + (int16_t)modeH / 2 + padding;
    }
    
    // Draw 1-pixel border around mode button
    gfx->drawRect(modeBtnLeft, modeBtnTop,
                  modeBtnRight - modeBtnLeft,
                  modeBtnBottom - modeBtnTop,
                  uiColor);
    
    // Draw "M" text centered inside the button
    int16_t modeBtnCenterY = (modeBtnTop + modeBtnBottom) / 2;
    int16_t modeBtnCenterX = (modeBtnLeft + modeBtnRight) / 2;
    drawCenteredText(modeTxt, modeBtnCenterX, modeBtnCenterY, uiColor, 3);
    
    modeBtnValid = true;
    lastDisplayedMode = currentMode;
    
    // Draw initial time text
    uint8_t textSize = showMinutesOnly ? 5 : 3;
    drawCenteredText(timeStr, centerX, centerY, uiColor, textSize);
    strcpy(lastTimeStr, timeStr);
    lastShowMinutesOnly = showMinutesOnly;
  } else {
    // Update progress circle - update more frequently for smoother animation
    drawProgressCircle(progress, centerX, centerY, radius, uiColor);
  }
  
  // Update time text if it changed or display mode changed
  if (strcmp(timeStr, lastTimeStr) != 0 || showMinutesOnly != lastShowMinutesOnly) {
    // Clear the text area inside the circle by filling a rectangle
    // Circle radius is 70, so we fill a safe rectangle inside it (100x60 pixels)
    int16_t textAreaWidth = 100;
    int16_t textAreaHeight = 60;
    int16_t textAreaLeft = centerX - textAreaWidth / 2;
    int16_t textAreaTop = centerY - textAreaHeight / 2;
    gfx->fillRect(textAreaLeft, textAreaTop, textAreaWidth, textAreaHeight, COLOR_BLACK);
    
    // Draw new time with current UI color and appropriate size
    uint8_t textSize = showMinutesOnly ? 5 : 3;  // Larger text for MM only mode
    drawCenteredText(timeStr, centerX, centerY, uiColor, textSize);
    strcpy(lastTimeStr, timeStr);
    lastShowMinutesOnly = showMinutesOnly;
  }
  
  // Update status text if state changed
  if (currentState != lastDisplayedState) {
    // Determine new status and color
    const char *statusTxt = nullptr;
    uint16_t statusColor = uiColor;  // Use current UI color (gold for work, blue for rest)
    bool useIcon = false;
    bool isPauseIcon = false;
    
    if (currentState == PAUSED) {
      useIcon = true;
      isPauseIcon = false;  // Play icon
    } else if (currentState == RUNNING) {
      useIcon = true;
      isPauseIcon = true;   // Pause icon
    } else if (isWorkSession) {
      statusTxt = "work";
    } else {
      statusTxt = "rest";
    }
    
    // Erase old status by drawing it in black (if we had one)
    if (lastDisplayedState != STOPPED) {
      // Erase old button area
      gfx->fillRect(statusBtnLeft, statusBtnTop,
                    statusBtnRight - statusBtnLeft,
                    statusBtnBottom - statusBtnTop,
                    COLOR_BLACK);
    }
    
    // Calculate button size
    int16_t x1, y1;
    uint16_t w, h;
    int16_t iconSize = 24;
    
    if (useIcon) {
      w = iconSize + 8;
      h = iconSize;
    } else {
      gfx->setFont(nullptr);
      gfx->setTextSize(3, 3, 0);
      gfx->getTextBounds(statusTxt, 0, 0, &x1, &y1, &w, &h);
    }

    int padding = 6;
    int16_t statusCenterX, statusCenterY;
    
    if (isLandscape) {
      // Landscape: status button on the right side
      statusCenterX = gfx->width() - 35;
      statusCenterY = gfx->height() / 2;
      statusBtnLeft   = statusCenterX - (int16_t)w / 2 - padding;
      statusBtnRight  = statusCenterX + (int16_t)w / 2 + padding;
      statusBtnTop    = statusCenterY - (int16_t)h / 2 - padding;
      statusBtnBottom = statusCenterY + (int16_t)h / 2 + padding;
    } else {
      // Portrait: status button at the bottom
      statusCenterX = gfx->width() / 2;
      statusCenterY = gfx->height() - 30;
      statusBtnLeft   = statusCenterX - (int16_t)w / 2 - padding;
      statusBtnRight  = statusCenterX + (int16_t)w / 2 + padding;
      statusBtnTop    = statusCenterY - (int16_t)h / 2 - padding;
      statusBtnBottom = statusCenterY + (int16_t)h / 2 + padding;
    }

    // Draw new border
    gfx->drawRect(statusBtnLeft, statusBtnTop,
                  statusBtnRight - statusBtnLeft,
                  statusBtnBottom - statusBtnTop,
                  statusColor);

    // Draw icon or text centered inside the button
    int16_t btnCenterY = (statusBtnTop + statusBtnBottom) / 2;
    int16_t btnCenterX = (statusBtnLeft + statusBtnRight) / 2;
    if (useIcon) {
      if (isPauseIcon) {
        drawPauseIcon(btnCenterX, btnCenterY, iconSize, statusColor);
      } else {
        drawPlayIcon(btnCenterX, btnCenterY, iconSize, statusColor);
      }
    } else {
      drawCenteredText(statusTxt, btnCenterX, btnCenterY, statusColor, 3);
    }
    statusBtnValid = true;
    lastDisplayedState = currentState;
  }
  
  // Update mode button if mode changed (always show "M" now, but still track mode changes for redraw)
  if (currentMode != lastDisplayedMode) {
    // Erase old mode button if it existed
    if (modeBtnValid) {
      gfx->fillRect(modeBtnLeft, modeBtnTop,
                    modeBtnRight - modeBtnLeft,
                    modeBtnBottom - modeBtnTop,
                    COLOR_BLACK);
    }
    
    // Draw "M" button (same as initial draw) - same size as pause/resume button
    const char *modeTxt = "M";
    
    gfx->setFont(nullptr);
    gfx->setTextSize(3, 3, 0);  // Same text size as status button
    
    // Use same size as status button (iconSize = 24, padding = 6)
    int16_t modeIconSize = 24;  // Same as status button icon size
    int16_t modeW = modeIconSize + 8;  // Same width calculation as status button (32)
    int16_t modeH = modeIconSize;  // Same height as status button (24)
    int padding = 6;  // Same padding as status button
    int16_t modeCenterX, modeCenterY;
    
    if (isLandscape) {
      // Landscape: mode button on the left side, vertically centered
      modeCenterX = 35;
      modeCenterY = gfx->height() / 2;
      modeBtnLeft   = modeCenterX - (int16_t)modeW / 2 - padding;
      modeBtnRight  = modeCenterX + (int16_t)modeW / 2 + padding;
      modeBtnTop    = modeCenterY - (int16_t)modeH / 2 - padding;
      modeBtnBottom = modeCenterY + (int16_t)modeH / 2 + padding;
    } else {
      // Portrait: mode button at the top center
      int16_t topMargin = 24;
      modeCenterX = gfx->width() / 2;
      modeCenterY = topMargin + (int16_t)modeH / 2 + padding;
      modeBtnLeft   = modeCenterX - (int16_t)modeW / 2 - padding;
      modeBtnRight  = modeCenterX + (int16_t)modeW / 2 + padding;
      modeBtnTop    = topMargin;
      modeBtnBottom = modeCenterY + (int16_t)modeH / 2 + padding;
    }
    
    // Draw new border
    gfx->drawRect(modeBtnLeft, modeBtnTop,
                  modeBtnRight - modeBtnLeft,
                  modeBtnBottom - modeBtnTop,
                  uiColor);
    
    // Draw "M" text centered inside the button
    int16_t modeBtnCenterY = (modeBtnTop + modeBtnBottom) / 2;
    int16_t modeBtnCenterX = (modeBtnLeft + modeBtnRight) / 2;
    drawCenteredText(modeTxt, modeBtnCenterX, modeBtnCenterY, uiColor, 3);
    
    modeBtnValid = true;
    lastDisplayedMode = currentMode;
  }
}

void drawProgressCircle(float progress, int centerX, int centerY, int radius, uint16_t color) {
  static float lastProgress = -1.0f;
  static bool circleDrawn = false;
  static uint16_t lastColor = COLOR_GOLD;
  int borderWidth = 5;
  
  // Force redraw on rotation change
  if (forceCircleRedraw) {
    circleDrawn = false;
    lastProgress = -1.0f;
    forceCircleRedraw = false;
  }
  
  // Redraw full circle if color changed (work <-> rest transition)
  if (lastColor != color) {
    circleDrawn = false;
    lastProgress = -1.0f;  // Force full redraw
    lastColor = color;
  }
  
  // Only redraw full circle on first call or if progress reset (timer restarted)
  if (!circleDrawn || progress < lastProgress || lastProgress < 0) {
    // Draw the full circle border with current color
    for (int16_t i = 0; i < borderWidth; i++) {
      gfx->drawCircle(centerX, centerY, radius - i, color);
    }
    circleDrawn = true;
    if (progress < lastProgress || lastProgress < 0) {
      lastProgress = 0.0f;  // Reset on timer restart
    }
  }
  
  // Only erase the newly elapsed portion (smooth incremental update)
  if (progress > lastProgress && lastProgress >= 0) {
    // Calculate how many new segments to erase
    const int segments = 720;  // More segments for smoother progress (2 per degree)
    int lastSegmentsErased = (int)(segments * lastProgress);
    int currentSegmentsErased = (int)(segments * progress);
    
    // Erase only the newly elapsed portion to avoid flickering
    for (int i = lastSegmentsErased; i < currentSegmentsErased; i++) {
      float angle = (i * 2.0f * PI) / segments - PI / 2.0f;  // Start from top
      // Erase all border layers
      for (int thickness = 0; thickness < borderWidth; thickness++) {
        int currentRadius = radius - thickness;
        if (currentRadius < 0) continue;
        
        // Erase a small arc segment to catch all circle pixels
        for (float angleOffset = -0.015f; angleOffset <= 0.015f; angleOffset += 0.005f) {
          float currentAngle = angle + angleOffset;
          int x = centerX + currentRadius * cosf(currentAngle);
          int y = centerY + currentRadius * sinf(currentAngle);
          gfx->drawPixel(x, y, COLOR_BLACK);
          // Erase neighboring pixels to ensure complete erasure
          gfx->drawPixel(x + 1, y, COLOR_BLACK);
          gfx->drawPixel(x - 1, y, COLOR_BLACK);
          gfx->drawPixel(x, y + 1, COLOR_BLACK);
          gfx->drawPixel(x, y - 1, COLOR_BLACK);
        }
      }
    }
  }
  
  lastProgress = progress;
}

void displayStoppedState() {
  drawSplash();
}
