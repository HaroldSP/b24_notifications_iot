// WiFi and Telegram bot implementation

#include "wifi_telegram.h"
#include "pomodoro_globals.h"
#include "display_updates.h"
#include "timer_logic.h"
#include "bitrix24.h"
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

// --- B24 group/project selection (simple state machine) ---
enum TelegramB24State : uint8_t {
  B24_STATE_IDLE = 0,
  B24_STATE_AWAIT_IDS = 1,
  B24_STATE_AWAIT_NEXT_ACTION = 2
};
static TelegramB24State b24State = B24_STATE_IDLE;
static char b24SelectedIds[192] = ""; // space-separated ids
static bool b24ShowDelayed = false;

static void telegramB24SetSingleGroup(const String& numericText) {
  uint32_t gid = numericText.toInt();
  if (gid == 0) return;
  setBitrixSelectedGroupId(gid);

  // Get current stats for this group (best-effort)
  uint16_t delayed = 0;
  uint16_t comments = 0;
  bitrixGetGroupStats(gid, &delayed, &comments);
  // Fallback: if comments came back as 0, try cached counts
  if (comments == 0) {
    Bitrix24Counts c = getBitrix24Counts();
    if (c.valid && c.groupComments > 0 && getBitrixSelectedGroupId() == gid) {
      comments = c.groupComments;
    }
  }

  // Optional name (for logging / user info)
  String name = bitrixGetGroupName(gid);

  // Console info
  Serial.print("[B24 GROUP] Selected ID=");
  Serial.print(gid);
  if (name.length() > 0) {
    Serial.print(" Name=\"");
    Serial.print(name);
    Serial.print("\"");
  }
  Serial.print(" Delayed=");
  Serial.print(delayed);
  Serial.print(" Comments=");
  Serial.println(comments);

  // Telegram info (single compact message)
  String msg = "<b>Group saved!</b>\n";
  msg += "ID: <b>";
  msg += String(gid);
  msg += "</b>";
  if (name.length() > 0) {
    msg += "\nName: <b>";
    msg += name;
    msg += "</b>";
  }
  msg += "\nDelayed tasks: <b>";
  msg += String(delayed);
  msg += "</b>";
  msg += "\nUnread comments: <b>";
  msg += String(comments);
  msg += "</b>";
  msg += "\n\nReply <b>ALL</b> to switch back to <b>ALL delayed-by-me</b> mode.\n";
  msg += "Or send another <b>group ID</b>.";
  sendTelegramMessage(msg);
}

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
// Use a larger buffer to avoid truncating multi-line / UTF-8 messages
struct TelegramMsg {
  char text[256];
};

// Last queued message to prevent duplicates
char lastQueuedMessage[256] = "";
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
  bot->sendMessage(chatId, "@office_b24_bot connected", "HTML");
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
        // send as HTML-formatted message (for bold parts)
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
        String rawText = text;  // keep original for names / IDs
        String from_id = bot->messages[i].chat_id;
        
        if (from_id != String(chatId)) continue;
        text.toLowerCase();
        
        Serial.print("[TG] Command: ");
        Serial.println(text);
        
        if (text == "/start" || text == "/help") {
          String msg = "📊 @office_b24_bot\n\n";
          msg += "Pomodoro:\n";
          msg += "/status - Current status\n";
          msg += "/work - Start work\n";
          msg += "/pause - Pause\n";
          msg += "/resume - Resume\n";
          msg += "/stop - Stop\n";
          msg += "/mode - Change mode\n\n";
          msg += "Bitrix24:\n";
          msg += "/b24groups - Configure groups/projects IDs\n";
          msg += "Notifications are sent when counts change";
          bot->sendMessage(chatId, msg, "HTML");
        }
        else if (text == "/b24groups") {
          b24State = B24_STATE_AWAIT_IDS;
          bot->sendMessage(chatId,
            "Send group/project ID (single group).\n"
            "Example: 253",
            "HTML"
          );
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
        else {
          // Accept plain numeric ID even if user didn't run /b24groups
          // (this matches your workflow: tap 3rd section -> send \"253\")
          if (b24State == B24_STATE_IDLE) {
            String digits;
            digits.reserve(16);
            for (uint16_t k = 0; k < rawText.length() && digits.length() < 16; k++) {
              char c = rawText[k];
              if (c >= '0' && c <= '9') digits += c;
            }
            digits.trim();
            String trimmedRaw = rawText;
            trimmedRaw.trim();
            if (digits.length() > 0 && digits.length() == trimmedRaw.length()) {
              digits.toCharArray(b24SelectedIds, sizeof(b24SelectedIds));
              telegramB24SetSingleGroup(digits);
              b24State = B24_STATE_AWAIT_NEXT_ACTION;
              continue;
            }
          }

          // --- B24 group/project flow replies ---
          if (b24State == B24_STATE_AWAIT_IDS) {
            // Only numeric IDs supported
            String cleaned = rawText;
            cleaned.trim();
            String digits;
            digits.reserve(16);
            for (uint16_t k = 0; k < cleaned.length() && digits.length() < 16; k++) {
              char c = cleaned[k];
              if (c >= '0' && c <= '9') digits += c;
            }
            digits.trim();

            if (digits.length() > 0 && digits.length() == cleaned.length()) {
              digits.toCharArray(b24SelectedIds, sizeof(b24SelectedIds));
              telegramB24SetSingleGroup(digits);
              b24State = B24_STATE_AWAIT_NEXT_ACTION;
              bot->sendMessage(chatId,
                "Tap the 3rd section again to switch back to ALL delayed-by-me mode.\n"
                "Or reply: ALL to switch back now.",
                "HTML"
              );
            } else {
              bot->sendMessage(chatId, "Only numeric group IDs are supported, e.g. 253.", "HTML");
            }
          } else if (b24State == B24_STATE_AWAIT_NEXT_ACTION) {
            if (text.indexOf("all") >= 0 || text.indexOf("ALL") >= 0) {
              setBitrixSelectedGroupId(0);
              b24State = B24_STATE_IDLE;
              bot->sendMessage(chatId, "OK. Switched back to ALL delayed-by-me mode.", "HTML");
            } else {
              // If user sends another number, treat as new group id
              String digits;
              digits.reserve(16);
              for (uint16_t k = 0; k < text.length() && digits.length() < 16; k++) {
                char c = text[k];
                if (c >= '0' && c <= '9') digits += c;
              }
              digits.trim();
              if (digits.length() > 0) {
                telegramB24SetSingleGroup(digits);
              } else {
                bot->sendMessage(chatId, "Reply ALL to switch back, or send another group ID.", "HTML");
              }
            }
          }
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
