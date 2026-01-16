// Timer control and state management implementation

#include "timer_logic.h"
#include "pomodoro_globals.h"
#include "pomodoro_config.h"
#include "wifi_telegram.h"
#include "display_updates.h"
#include "color_utils.h"

// Last telegram send time to prevent duplicates
static unsigned long lastTgSendTime = 0;
const unsigned long TG_SEND_DEBOUNCE = 3000;  // 3 seconds

void startTimer() {
  if (currentState == RUNNING) return;
  Serial.println("[TIMER] startTimer called");
  currentState = RUNNING;
  isWorkSession = true;
  startTime = millis();
  timerStartTime = millis();
  elapsedBeforePause = 0;
  displayInitialized = false;
  forceCircleRedraw = true;
  if (millis() - lastTgSendTime > TG_SEND_DEBOUNCE) {
    lastTgSendTime = millis();
    sendTelegramMessage("🍅 <b>Work started!</b>");
  }
}

void pauseTimer() {
  if (currentState != RUNNING) return;
  Serial.println("[TIMER] pauseTimer called");
  currentState = PAUSED;
  pausedTime = millis();
  elapsedBeforePause = millis() - startTime;
  displayInitialized = false;
  forceCircleRedraw = true;
  if (millis() - lastTgSendTime > TG_SEND_DEBOUNCE) {
    lastTgSendTime = millis();
    sendTelegramMessage("⏸ <b>Timer paused</b>");
  }
}

void resumeTimer() {
  if (currentState != PAUSED) return;
  Serial.println("[TIMER] resumeTimer called");
  currentState = RUNNING;
  startTime = millis() - elapsedBeforePause;
  displayInitialized = false;
  forceCircleRedraw = true;
  if (millis() - lastTgSendTime > TG_SEND_DEBOUNCE) {
    lastTgSendTime = millis();
    sendTelegramMessage("▶️ <b>Timer resumed</b>");
  }
}

void stopTimer() {
  if (currentState == STOPPED) return;
  Serial.println("[TIMER] stopTimer called");
  currentState = STOPPED;
  displayInitialized = false;
  if (millis() - lastTgSendTime > TG_SEND_DEBOUNCE) {
    lastTgSendTime = millis();
    sendTelegramMessage("⏹ <b>Timer stopped</b>");
  }
  displayStoppedState();
}

// Helper function to get current duration based on mode
unsigned long getCurrentDuration() {
  if (isWorkSession) {
    switch (currentMode) {
      case MODE_1_1:  return POMODORO_DURATION_1;
      case MODE_25_5: return POMODORO_DURATION_25;
      case MODE_50_10: return POMODORO_DURATION_50;
      default: return POMODORO_DURATION_25;
    }
  } else {
    switch (currentMode) {
      case MODE_1_1:  return REST_DURATION_1;
      case MODE_25_5: return REST_DURATION_5;
      case MODE_50_10: return REST_DURATION_10;
      default: return REST_DURATION_5;
    }
  }
}

void updateTimer() {
  // Only update timer if it's actually running and we're not on home/preview screens
  if (currentState == RUNNING && currentViewMode == 0) {
    unsigned long elapsed = millis() - startTime;
    unsigned long duration = getCurrentDuration();
    if (elapsed >= duration) {
      if (isWorkSession) {
        isWorkSession = false;
        startTime = millis();
        displayInitialized = false;  // Force redraw to update colors
        // Send Telegram notification
        sendTelegramMessage("☕ <b>Rest time!</b> Take a break.");
      } else {
        isWorkSession = true;
        startTime = millis();
        displayInitialized = false;  // Force redraw to update colors
        // Send Telegram notification
        sendTelegramMessage("🍅 <b>Work time!</b> Focus on your task.");
      }
    }
  }
}

// Helper function to get current UI color based on work/rest session
uint16_t getCurrentUIColor() {
  if (isWorkSession) {
    return selectedWorkColor;  // Use user-selected color for work
  } else {
    // Use selected rest color if set, otherwise use inverted work color
    if (selectedRestColor != 0) {
      return selectedRestColor;
    } else {
      return invertColor(selectedWorkColor);  // Inverted color for rest
    }
  }
}
