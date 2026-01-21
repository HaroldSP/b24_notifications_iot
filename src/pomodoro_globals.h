// Global variables and forward declarations

#ifndef POMODORO_GLOBALS_H
#define POMODORO_GLOBALS_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <Preferences.h>
#include "pomodoro_types.h"
#include "pomodoro_config.h"

// Forward declarations
extern Arduino_GFX *gfx;
extern Arduino_DataBus *bus;
extern Preferences preferences;

// Timer state
extern TimerState currentState;
extern PomodoroMode currentMode;
extern uint8_t currentViewMode;
extern bool gridViewActive;
extern bool isWorkSession;
extern unsigned long startTime;
extern unsigned long pausedTime;
extern unsigned long elapsedBeforePause;
extern unsigned long timerStartTime;

// Display state
extern bool displayInitialized;
extern char lastTimeStr[6];
extern TimerState lastDisplayedState;
extern bool showMinutesOnly;
extern bool lastShowMinutesOnly;
extern PomodoroMode lastDisplayedMode;
extern bool forceCircleRedraw;

// Color selection
extern uint16_t selectedWorkColor;
extern uint16_t selectedRestColor;
extern uint16_t tempPreviewColor;
extern uint16_t tempPreviewRestColor;
extern int8_t tempSelectedColorIndex;
extern bool selectingRestColor;

// Grid parameters
extern int16_t gridCellWidth;
extern int16_t gridCellHeight;
extern int16_t gridStartX;
extern int16_t gridNumRows;
extern int16_t gridNumCols;
extern int16_t lastSelectedGridRow;
extern int16_t lastSelectedGridCol;

// Rotation
extern uint8_t currentRotation;
extern unsigned long lastRotationCheck;

// Button bounds (declared in respective modules but needed globally)
extern int16_t statusBtnLeft, statusBtnRight, statusBtnTop, statusBtnBottom;
extern bool statusBtnValid;
extern int16_t modeBtnLeft, modeBtnRight, modeBtnTop, modeBtnBottom;
extern bool modeBtnValid;
extern int16_t gearBtnLeft, gearBtnRight, gearBtnTop, gearBtnBottom;
extern bool gearBtnValid;
extern int16_t gridCancelBtnLeft, gridCancelBtnRight, gridCancelBtnTop, gridCancelBtnBottom;
extern bool gridCancelBtnValid;
extern int16_t gridConfirmBtnLeft, gridConfirmBtnRight, gridConfirmBtnTop, gridConfirmBtnBottom;
extern bool gridConfirmBtnValid;
extern int16_t previewCancelBtnLeft, previewCancelBtnRight, previewCancelBtnTop, previewCancelBtnBottom;
extern bool previewCancelBtnValid;
extern int16_t previewConfirmBtnLeft, previewConfirmBtnRight, previewConfirmBtnTop, previewConfirmBtnBottom;
extern bool previewConfirmBtnValid;
extern int16_t previewRestSwatchLeft, previewRestSwatchRight, previewRestSwatchTop, previewRestSwatchBottom;
extern bool previewRestSwatchValid;
extern int16_t previewWorkSwatchLeft, previewWorkSwatchRight, previewWorkSwatchTop, previewWorkSwatchBottom;
extern bool previewWorkSwatchValid;
extern int16_t workBtnLeft, workBtnRight, workBtnTop, workBtnBottom;
extern bool workBtnValid;
extern int16_t restBtnLeft, restBtnRight, restBtnTop, restBtnBottom;
extern bool restBtnValid;
// Main functionality screen buttons
extern int16_t mainMenuB24BtnLeft, mainMenuB24BtnRight, mainMenuB24BtnTop, mainMenuB24BtnBottom;
extern bool mainMenuB24BtnValid;
extern int16_t mainMenuTomatoBtnLeft, mainMenuTomatoBtnRight, mainMenuTomatoBtnTop, mainMenuTomatoBtnBottom;
extern bool mainMenuTomatoBtnValid;
extern int16_t mainMenuPaletteBtnLeft, mainMenuPaletteBtnRight, mainMenuPaletteBtnTop, mainMenuPaletteBtnBottom;
extern bool mainMenuPaletteBtnValid;
extern int16_t mainMenuAPBtnLeft, mainMenuAPBtnRight, mainMenuAPBtnTop, mainMenuAPBtnBottom;
extern bool mainMenuAPBtnValid;
extern bool apEnabled;

// Color palette
extern const uint16_t paletteColors[];
extern const int paletteSize;

// Touch handler variables
extern int16_t lastTouchX;
extern int16_t lastTouchY;
extern bool lastTouchValid;
extern bool tapIndicatorActive;
extern unsigned long tapIndicatorStart;
extern int16_t tapIndicatorX;
extern int16_t tapIndicatorY;

// Temporary Telegram prompt screen timer (for B24 -> TG prompt UX)
extern unsigned long tgPromptUntilMs;
// Temporary AP prompt screen timer (for AP mode activation)
extern unsigned long apPromptUntilMs;

#endif // POMODORO_GLOBALS_H
