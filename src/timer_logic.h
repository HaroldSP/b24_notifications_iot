// Timer control and state management

#ifndef TIMER_LOGIC_H
#define TIMER_LOGIC_H

#include <Arduino.h>
#include "pomodoro_types.h"

// Timer control functions
void startTimer();
void pauseTimer();
void resumeTimer();
void stopTimer();
void updateTimer();

// Helper functions
unsigned long getCurrentDuration();
uint16_t getCurrentUIColor();

#endif // TIMER_LOGIC_H
