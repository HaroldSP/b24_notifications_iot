// Persistent storage implementation

#include "storage.h"
#include "pomodoro_globals.h"

// Save selected color to NVS (persistent storage)
void saveSelectedColor() {
  preferences.begin("pomodoro", false);  // Open namespace in RW mode
  preferences.putUShort("workColor", selectedWorkColor);
  preferences.putUShort("restColor", selectedRestColor);  // Save rest color (0 = use inverted)
  preferences.end();
  Serial.print("Saved work color to NVS: 0x");
  Serial.println(selectedWorkColor, HEX);
  if (selectedRestColor != 0) {
    Serial.print("Saved rest color to NVS: 0x");
    Serial.println(selectedRestColor, HEX);
  } else {
    Serial.println("Rest color: using inverted work color");
  }
}

// Load selected color from NVS (persistent storage)
void loadSelectedColor() {
  preferences.begin("pomodoro", true);  // Open namespace in read-only mode
  selectedWorkColor = preferences.getUShort("workColor", COLOR_GOLD);  // Default to gold
  selectedRestColor = preferences.getUShort("restColor", 0);  // Default to 0 (use inverted work color)
  preferences.end();
  Serial.print("Loaded work color from NVS: 0x");
  Serial.println(selectedWorkColor, HEX);
  if (selectedRestColor != 0) {
    Serial.print("Loaded rest color from NVS: 0x");
    Serial.println(selectedRestColor, HEX);
  } else {
    Serial.println("Rest color: using inverted work color");
  }
}
