// WiFi and Telegram bot implementation

#include "wifi_telegram.h"
#include "pomodoro_globals.h"
#include "display_updates.h"
#include "timer_logic.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// WiFi and Telegram state
bool wifiConnected = false;
bool telegramConfigured = false;

// Use build flags for bot token and chat_id
const char* botToken = TELEGRAM_BOT_TOKEN;
const char* chatId = TELEGRAM_CHAT_ID;

// WiFi client for Telegram
WiFiClientSecure telegramClient;
UniversalTelegramBot* bot = nullptr;

// FreeRTOS task handle for Telegram
TaskHandle_t telegramTaskHandle = nullptr;

// Thread-safe command queue from Telegram to main loop
volatile bool telegramCmdStart = false;
volatile bool telegramCmdPause = false;
volatile bool telegramCmdResume = false;
volatile bool telegramCmdStop = false;
volatile bool telegramCmdMode = false;

// Outgoing message queue (main loop -> telegram task)
QueueHandle_t telegramMsgQueue = nullptr;
struct TelegramMsg {
  char text[128];
};

// Last queued message to prevent duplicates
char lastQueuedMessage[128] = "";
unsigned long lastSentTime = 0;

// Mutex for telegram operations
SemaphoreHandle_t telegramMutex = nullptr;

// Connect to WiFi
void connectWiFi() {
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println();
    Serial.println("WiFi connection failed!");
  }
}

// Initialize Telegram bot
void initTelegramBot() {
  // Check if bot token is configured
  telegramConfigured = (strlen(botToken) > 0 && strlen(chatId) > 0);
  
  if (!wifiConnected || !telegramConfigured) {
    Serial.println("Telegram not configured or WiFi not connected");
    return;
  }
  
  if (bot != nullptr) {
    delete bot;
  }
  
  telegramClient.setInsecure();  // Skip certificate verification
  telegramClient.setTimeout(10000);  // 10 second timeout to prevent retries
  bot = new UniversalTelegramBot(botToken, telegramClient);
  bot->waitForResponse = 5000;  // 5 second wait for response
  Serial.println("Telegram bot initialized");
  
  // Send startup message
  bot->sendMessage(chatId, "🍅 Pomodoro Timer connected!", "HTML");
}

// Queue message to Telegram (non-blocking)
void sendTelegramMessage(const String& message) {
  if (!wifiConnected || !telegramConfigured || telegramMsgQueue == nullptr) {
    return;
  }
  
  TelegramMsg msg;
  message.toCharArray(msg.text, sizeof(msg.text));
  
  if (xQueueSend(telegramMsgQueue, &msg, 0) == pdTRUE) {
    Serial.print("[TG] Queued: ");
    Serial.println(message);
  }
}

// Telegram task - sends queued messages in background
void telegramTask(void* parameter) {
  Serial.println("[TG TASK] Started");
  
  while (true) {
    // Send queued messages
    TelegramMsg outMsg;
    if (telegramMsgQueue != nullptr && xQueueReceive(telegramMsgQueue, &outMsg, 0) == pdTRUE) {
      if (bot != nullptr) {
        Serial.print("[TG TASK] Sending: ");
        Serial.println(outMsg.text);
        bot->sendMessage(chatId, outMsg.text, "HTML");
        Serial.println("[TG TASK] Done");
      }
    }
    
    // Check for incoming commands (less frequently)
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > BOT_CHECK_INTERVAL && bot != nullptr) {
      lastCheck = millis();
      int numNewMessages = bot->getUpdates(bot->last_message_received + 1);
      
      for (int i = 0; i < numNewMessages; i++) {
        String text = bot->messages[i].text;
        String from_id = bot->messages[i].chat_id;
        
        if (from_id != String(chatId)) continue;
        text.toLowerCase();
        
        Serial.print("[TG] Command: ");
        Serial.println(text);
        
        if (text == "/start" || text == "/help") {
          String msg = "🍅 <b>Pomodoro Timer</b>\n\n";
          msg += "/status - Current status\n";
          msg += "/work - Start work\n";
          msg += "/pause - Pause\n";
          msg += "/resume - Resume\n";
          msg += "/stop - Stop\n";
          msg += "/mode - Change mode";
          bot->sendMessage(chatId, msg, "HTML");
        }
        else if (text == "/work") {
          telegramCmdStart = true;
          bot->sendMessage(chatId, "🍅 Starting...", "HTML");
        }
        else if (text == "/pause") {
          telegramCmdPause = true;
          bot->sendMessage(chatId, "⏸ Pausing...", "HTML");
        }
        else if (text == "/resume") {
          telegramCmdResume = true;
          bot->sendMessage(chatId, "▶️ Resuming...", "HTML");
        }
        else if (text == "/stop") {
          telegramCmdStop = true;
          bot->sendMessage(chatId, "⏹ Stopping...", "HTML");
        }
        else if (text == "/mode") {
          telegramCmdMode = true;
          String modeStr;
          switch (currentMode) {
            case MODE_1_1: modeStr = "25/5"; break;
            case MODE_25_5: modeStr = "50/10"; break;
            case MODE_50_10: modeStr = "1/1"; break;
          }
          bot->sendMessage(chatId, "⏱ Mode: " + modeStr, "HTML");
        }
        else if (text == "/status") {
          String msg = "🍅 ";
          msg += (currentState == STOPPED) ? "Stopped" : 
                 (currentState == RUNNING) ? (isWorkSession ? "Working" : "Resting") : "Paused";
          msg += " | ";
          switch (currentMode) {
            case MODE_1_1: msg += "1/1"; break;
            case MODE_25_5: msg += "25/5"; break;
            case MODE_50_10: msg += "50/10"; break;
          }
          bot->sendMessage(chatId, msg, "HTML");
        }
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));  // Check queue frequently
  }
}

// Process Telegram commands in main loop (thread-safe)
void processTelegramCommands() {
  if (telegramCmdStart) {
    telegramCmdStart = false;
    if (currentState == STOPPED) {
      Serial.println("[TG CMD] Starting timer");
      startTimer();
    }
  }
  if (telegramCmdPause) {
    telegramCmdPause = false;
    if (currentState == RUNNING) {
      Serial.println("[TG CMD] Pausing timer");
      pauseTimer();
    }
  }
  if (telegramCmdResume) {
    telegramCmdResume = false;
    if (currentState == PAUSED) {
      Serial.println("[TG CMD] Resuming timer");
      resumeTimer();
    }
  }
  if (telegramCmdStop) {
    telegramCmdStop = false;
    if (currentState != STOPPED) {
      Serial.println("[TG CMD] Stopping timer");
      stopTimer();
    }
  }
  if (telegramCmdMode) {
    telegramCmdMode = false;
    Serial.println("[TG CMD] Changing mode");
    switch (currentMode) {
      case MODE_1_1: currentMode = MODE_25_5; break;
      case MODE_25_5: currentMode = MODE_50_10; break;
      case MODE_50_10: currentMode = MODE_1_1; break;
    }
    displayInitialized = false;
    forceCircleRedraw = true;
  }
}

// Start Telegram task on separate core
void startTelegramTask() {
  if (!wifiConnected || !telegramConfigured) return;
  
  // Create mutex for thread-safe telegram operations
  telegramMutex = xSemaphoreCreateMutex();
  
  // Create message queue for outgoing messages
  telegramMsgQueue = xQueueCreate(MSG_QUEUE_SIZE, sizeof(TelegramMsg));
  
  // Create task with low priority (but not lowest)
  xTaskCreatePinnedToCore(
    telegramTask,           // Task function
    "TelegramTask",         // Task name
    8192,                   // Stack size
    NULL,                   // Parameters
    1,                      // Priority (low but runs)
    &telegramTaskHandle,    // Task handle
    0                       // Core 0
  );
  Serial.println("Telegram task created on core 0");
}
