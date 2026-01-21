// Common types and enums for Pomodoro timer

#ifndef POMODORO_TYPES_H
#define POMODORO_TYPES_H

#include <Arduino.h>

// Timer states
enum TimerState {
  STOPPED,
  RUNNING,
  PAUSED
};

// Pomodoro modes
enum PomodoroMode {
  MODE_1_1,    // 1 minute work, 1 minute rest (for testing)
  MODE_25_5,   // 25 minutes work, 5 minutes rest (standard)
  MODE_50_10   // 50 minutes work, 10 minutes rest (extended)
};

// View modes
#define VIEW_MODE_HOME 0
#define VIEW_MODE_GRID 1
#define VIEW_MODE_PREVIEW 2
#define VIEW_MODE_MAIN_MENU 3
#define VIEW_MODE_B24 4
#define VIEW_MODE_TG_PROMPT 5
#define VIEW_MODE_AP_PROMPT 6

#endif // POMODORO_TYPES_H
