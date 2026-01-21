// Global variable definitions

#include "pomodoro_globals.h"
#include "pomodoro_config.h"
#include "color_utils.h"
#include <Arduino_GFX_Library.h>

// Display objects
Arduino_DataBus *bus = new Arduino_HWSPI(15 /* DC */, 14 /* CS */, 1 /* SCK */, 2 /* MOSI */);
Arduino_GFX *gfx = new Arduino_ST7789(
  bus, 22 /* RST */, 0 /* rotation */, false /* IPS */,
  172 /* width */, 320 /* height */,
  34 /*col_offset1*/, 0 /*uint8_t row_offset1*/,
  34 /*col_offset2*/, 0 /*row_offset2*/);

Preferences preferences;

// Timer state
TimerState currentState = STOPPED;
PomodoroMode currentMode = MODE_25_5;
uint8_t currentViewMode = 0;
bool gridViewActive = false;
bool isWorkSession = true;
unsigned long startTime = 0;
unsigned long pausedTime = 0;
unsigned long elapsedBeforePause = 0;
unsigned long timerStartTime = 0;

// Display state
bool displayInitialized = false;
char lastTimeStr[6] = "";
TimerState lastDisplayedState = STOPPED;
bool showMinutesOnly = false;
bool lastShowMinutesOnly = false;
PomodoroMode lastDisplayedMode = MODE_25_5;
bool forceCircleRedraw = false;

// Color selection
uint16_t selectedWorkColor = COLOR_GOLD;
uint16_t selectedRestColor = 0;
uint16_t tempPreviewColor = COLOR_GOLD;
uint16_t tempPreviewRestColor = 0;
int8_t tempSelectedColorIndex = -1;
bool selectingRestColor = false;

// Grid parameters
int16_t gridCellWidth = 43;
int16_t gridCellHeight = 43;
int16_t gridStartX = 0;
int16_t gridNumRows = 7;
int16_t gridNumCols = 3;
int16_t lastSelectedGridRow = -1;
int16_t lastSelectedGridCol = -1;

// Rotation
uint8_t currentRotation = 0;
unsigned long lastRotationCheck = 0;

// Button bounds
int16_t statusBtnLeft = 0, statusBtnRight = 0, statusBtnTop = 0, statusBtnBottom = 0;
bool statusBtnValid = false;
int16_t modeBtnLeft = 0, modeBtnRight = 0, modeBtnTop = 0, modeBtnBottom = 0;
bool modeBtnValid = false;
int16_t gearBtnLeft = 0, gearBtnRight = 0, gearBtnTop = 0, gearBtnBottom = 0;
bool gearBtnValid = false;
int16_t gridCancelBtnLeft = 0, gridCancelBtnRight = 0, gridCancelBtnTop = 0, gridCancelBtnBottom = 0;
bool gridCancelBtnValid = false;
int16_t gridConfirmBtnLeft = 0, gridConfirmBtnRight = 0, gridConfirmBtnTop = 0, gridConfirmBtnBottom = 0;
bool gridConfirmBtnValid = false;
int16_t previewCancelBtnLeft = 0, previewCancelBtnRight = 0, previewCancelBtnTop = 0, previewCancelBtnBottom = 0;
bool previewCancelBtnValid = false;
int16_t previewConfirmBtnLeft = 0, previewConfirmBtnRight = 0, previewConfirmBtnTop = 0, previewConfirmBtnBottom = 0;
bool previewConfirmBtnValid = false;
int16_t previewRestSwatchLeft = 0, previewRestSwatchRight = 0, previewRestSwatchTop = 0, previewRestSwatchBottom = 0;
bool previewRestSwatchValid = false;
int16_t previewWorkSwatchLeft = 0, previewWorkSwatchRight = 0, previewWorkSwatchTop = 0, previewWorkSwatchBottom = 0;
bool previewWorkSwatchValid = false;

// Unused button bounds (kept for compatibility)
int16_t workBtnLeft = 0, workBtnRight = 0, workBtnTop = 0, workBtnBottom = 0;
bool workBtnValid = false;
int16_t restBtnLeft = 0, restBtnRight = 0, restBtnTop = 0, restBtnBottom = 0;
bool restBtnValid = false;
// Main functionality screen buttons
int16_t mainMenuB24BtnLeft = 0, mainMenuB24BtnRight = 0, mainMenuB24BtnTop = 0, mainMenuB24BtnBottom = 0;
bool mainMenuB24BtnValid = false;
int16_t mainMenuTomatoBtnLeft = 0, mainMenuTomatoBtnRight = 0, mainMenuTomatoBtnTop = 0, mainMenuTomatoBtnBottom = 0;
bool mainMenuTomatoBtnValid = false;
int16_t mainMenuPaletteBtnLeft = 0, mainMenuPaletteBtnRight = 0, mainMenuPaletteBtnTop = 0, mainMenuPaletteBtnBottom = 0;
bool mainMenuPaletteBtnValid = false;
int16_t mainMenuAPBtnLeft = 0, mainMenuAPBtnRight = 0, mainMenuAPBtnTop = 0, mainMenuAPBtnBottom = 0;
bool mainMenuAPBtnValid = false;
bool apEnabled = false;

// Touch handler variables
int16_t lastTouchX = 0;
int16_t lastTouchY = 0;
bool lastTouchValid = false;
static bool lastIntState = HIGH;
static unsigned long lastTpIntLowTime = 0;
bool tapIndicatorActive = false;
unsigned long tapIndicatorStart = 0;
int16_t tapIndicatorX = 0;
int16_t tapIndicatorY = 0;

// Temporary Telegram prompt screen timer (for B24 -> TG prompt UX)
unsigned long tgPromptUntilMs = 0;
// Temporary AP prompt screen timer (for AP mode activation)
unsigned long apPromptUntilMs = 0;
