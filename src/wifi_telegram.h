// WiFi and Telegram bot functionality

#ifndef WIFI_TELEGRAM_H
#define WIFI_TELEGRAM_H

#include <Arduino.h>
#include "pomodoro_types.h"

// WiFi credentials from platformio.ini build flags
#ifndef WIFI_SSID
  #define WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
  #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif
#ifndef TELEGRAM_BOT_TOKEN
  #define TELEGRAM_BOT_TOKEN ""
#endif
#ifndef TELEGRAM_CHAT_ID
  #define TELEGRAM_CHAT_ID ""
#endif

// Telegram bot polling interval
#define MSG_QUEUE_SIZE 3
const unsigned long BOT_CHECK_INTERVAL = 5000;  // Check every 5 seconds
const unsigned long SEND_COOLDOWN = 3000;  // 3 second cooldown between sends

// Thread-safe command queue from Telegram to main loop
extern volatile bool telegramCmdStart;
extern volatile bool telegramCmdPause;
extern volatile bool telegramCmdResume;
extern volatile bool telegramCmdStop;
extern volatile bool telegramCmdMode;

// Functions
void connectWiFi();
void initTelegramBot();
void sendTelegramMessage(const String& message);
void processTelegramCommands();
void startTelegramTask();

#endif // WIFI_TELEGRAM_H
