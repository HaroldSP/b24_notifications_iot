// Persistent storage (NVS) for settings

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include "pomodoro_config.h"

// Save selected color to NVS (persistent storage)
void saveSelectedColor();

// Load selected color from NVS (persistent storage)
void loadSelectedColor();

// Save WiFi credentials to NVS
void saveWiFiCredentials(const char* ssid, const char* password);

// Load WiFi credentials from NVS (returns true if found)
bool loadWiFiCredentials(char* ssid, size_t ssidLen, char* password, size_t passwordLen);

// Save Telegram credentials to NVS
void saveTelegramCredentials(const char* botToken, const char* chatId);

// Load Telegram credentials from NVS (returns true if found)
bool loadTelegramCredentials(char* botToken, size_t tokenLen, char* chatId, size_t chatIdLen);

// Save Bitrix24 credentials to NVS
void saveBitrix24Credentials(const char* hostname, const char* restEndpoint);

// Load Bitrix24 credentials from NVS (returns true if found)
bool loadBitrix24Credentials(char* hostname, size_t hostnameLen, char* restEndpoint, size_t endpointLen);

#endif // STORAGE_H
