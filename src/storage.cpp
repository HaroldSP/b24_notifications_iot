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

// Save WiFi credentials to NVS
void saveWiFiCredentials(const char* ssid, const char* password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
  Serial.println("WiFi credentials saved to NVS");
}

// Load WiFi credentials from NVS
bool loadWiFiCredentials(char* ssid, size_t ssidLen, char* password, size_t passwordLen) {
  if (!preferences.begin("wifi", true)) {
    // Namespace doesn't exist yet (first boot) - this is normal
    return false;
  }
  String ssidStr = preferences.getString("ssid", "");
  String passwordStr = preferences.getString("password", "");
  preferences.end();
  
  if (ssidStr.length() > 0) {
    strncpy(ssid, ssidStr.c_str(), ssidLen - 1);
    ssid[ssidLen - 1] = '\0';
    strncpy(password, passwordStr.c_str(), passwordLen - 1);
    password[passwordLen - 1] = '\0';
    Serial.println("WiFi credentials loaded from NVS");
    return true;
  }
  return false;
}

// Save Telegram credentials to NVS
void saveTelegramCredentials(const char* botToken, const char* chatId) {
  preferences.begin("telegram", false);
  preferences.putString("botToken", botToken);
  preferences.putString("chatId", chatId);
  preferences.end();
  Serial.println("Telegram credentials saved to NVS");
}

// Load Telegram credentials from NVS
bool loadTelegramCredentials(char* botToken, size_t tokenLen, char* chatId, size_t chatIdLen) {
  if (!preferences.begin("telegram", true)) {
    // Namespace doesn't exist yet (first boot) - this is normal
    return false;
  }
  String tokenStr = preferences.getString("botToken", "");
  String chatIdStr = preferences.getString("chatId", "");
  preferences.end();
  
  if (tokenStr.length() > 0 && chatIdStr.length() > 0) {
    strncpy(botToken, tokenStr.c_str(), tokenLen - 1);
    botToken[tokenLen - 1] = '\0';
    strncpy(chatId, chatIdStr.c_str(), chatIdLen - 1);
    chatId[chatIdLen - 1] = '\0';
    Serial.println("Telegram credentials loaded from NVS");
    return true;
  }
  return false;
}

// Save Bitrix24 credentials to NVS
void saveBitrix24Credentials(const char* hostname, const char* restEndpoint) {
  preferences.begin("bitrix24", false);
  preferences.putString("hostname", hostname);
  preferences.putString("restEndpoint", restEndpoint);
  preferences.end();
  Serial.println("Bitrix24 credentials saved to NVS");
}

// Load Bitrix24 credentials from NVS
bool loadBitrix24Credentials(char* hostname, size_t hostnameLen, char* restEndpoint, size_t endpointLen) {
  if (!preferences.begin("bitrix24", true)) {
    // Namespace doesn't exist yet (first boot) - this is normal
    return false;
  }
  String hostnameStr = preferences.getString("hostname", "");
  String endpointStr = preferences.getString("restEndpoint", "");
  preferences.end();
  
  if (hostnameStr.length() > 0 && endpointStr.length() > 0) {
    strncpy(hostname, hostnameStr.c_str(), hostnameLen - 1);
    hostname[hostnameLen - 1] = '\0';
    strncpy(restEndpoint, endpointStr.c_str(), endpointLen - 1);
    restEndpoint[endpointLen - 1] = '\0';
    Serial.println("Bitrix24 credentials loaded from NVS");
    return true;
  }
  return false;
}
