# Application Screens

## 1. Home/Stopped Screen

**Function:** `drawSplash()` / `displayStoppedState()`

Main screen when timer is stopped. Displays:

- Golden "R" logo in a circular border (color matches selected work color)
- Gear icon button (settings) - opens main functionality screen
- Tap on circle area to start timer

**Navigation:**

- Tap circle → Start timer
- Tap gear icon → Open main functionality screen

---

## 2. Timer Running Screen

**Function:** `drawTimer()` (when `currentState == RUNNING`)

Active timer display showing:

- Circular progress indicator (filled border that erases as time elapses)
- Time display (MM:SS or MM only, toggleable by tapping circle)
- Pause button (right side in landscape, bottom center in portrait)
- Mode button "M" (left side in landscape, top center in portrait)
- Color changes based on work/rest session (gold for work, blue for rest)

**Navigation:**

- Tap pause button → Pause timer
- Tap mode button "M" → Cycle through timer modes (1/1, 25/5, 50/10)
- Tap circle → Toggle time display format (MM:SS ↔ MM)

---

## 3. Timer Paused Screen

**Function:** `drawTimer()` (when `currentState == PAUSED`)

Same layout as running screen but with:

- Play button instead of pause button
- Timer display shows remaining time
- Progress circle shows current progress (frozen)

**Navigation:**

- Tap play button → Resume timer
- Tap mode button "M" → Cycle through timer modes
- Tap circle → Toggle time display format

---

## 4. Color Grid/Palette Screen

**Function:** `drawGrid()`

Color selection grid showing:

- Grid of color swatches (3 columns in portrait, 5 columns in landscape)
- Selected color highlighted with white border
- Cancel button "X" and Confirm button "V"
- Buttons positioned at bottom row (portrait) or right side (landscape)

**Navigation:**

- Tap color swatch → Select color (highlighted with white border)
- Tap "V" button → Confirm selection and return to home
- Tap "X" button → Cancel and return to home without saving

**Note:** This screen is accessed from color preview screen when selecting a color.

---

## 5. Color Preview Screen

**Function:** `drawColorPreview()`

Theme color preview and selection screen showing:

- "WORK" label and color swatch (clickable)
- "REST" label and color swatch (clickable)
- Cancel button "X" and Confirm button "V"
- Preview shows how selected colors will look

**Navigation:**

- Tap "WORK" swatch → Open color grid to select work color
- Tap "REST" swatch → Open color grid to select rest color
- Tap "V" button → Save colors and return to home
- Tap "X" button → Cancel and return to home without saving

**Note:** Accessed from main functionality screen by tapping color palette icon.

---

## 6. Main Functionality Screen

**Function:** `drawMainFunctionality()` (to be implemented)

Main menu screen with three action buttons:

- "B24" button - opens Bitrix24 notifications screen
- Tomato icon button - opens standard timer (home screen)
- Color palette icon (mdi mdi-palette) - opens color preview screen

**Navigation:**

- Tap "B24" button → Open B24 placeholder screen
- Tap tomato icon → Open home/timer screen
- Tap color palette icon → Open color preview screen

**Note:** Accessed from home screen by tapping gear icon.

---

## 7. B24 Placeholder Screen

**Function:** `drawB24Placeholder()` (to be implemented)

Placeholder screen for Bitrix24 notifications functionality. This screen will display Bitrix24 notifications and related information.

**Navigation:**

- Back button or gesture → Return to main functionality screen

**Note:** Accessed from main functionality screen by tapping "B24" button.

---

## View Modes

The application uses `currentViewMode` to track which screen is active:

- `VIEW_MODE_HOME (0)` - Home/stopped screen or timer screen
- `VIEW_MODE_GRID (1)` - Color grid/palette screen
- `VIEW_MODE_PREVIEW (2)` - Color preview screen
- `VIEW_MODE_MAIN_MENU (3)` - Main functionality screen
- `VIEW_MODE_B24 (4)` - B24 placeholder screen

---

## Screen Transitions

1. **Home → Timer:** Tap circle on home screen
2. **Timer → Home:** Timer completes or manually stopped
3. **Home → Main Functionality:** Tap gear icon
4. **Main Functionality → B24:** Tap "B24" button
5. **Main Functionality → Home/Timer:** Tap tomato icon
6. **Main Functionality → Color Preview:** Tap color palette icon
7. **B24 → Main Functionality:** Back button or gesture
8. **Color Preview → Color Grid:** Tap WORK or REST swatch
9. **Color Grid → Color Preview:** Tap "V" (confirm) or "X" (cancel)
10. **Color Preview → Main Functionality:** Tap "V" (save) or "X" (cancel)
