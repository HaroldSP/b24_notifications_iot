// Bitrix24 API integration implementation

#include "bitrix24.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#ifndef BITRIX_HOSTNAME
#define BITRIX_HOSTNAME "https://npfreom.bitrix24.ru"
#endif

#ifndef BITRIX_REST_ENDPOINT
#define BITRIX_REST_ENDPOINT "/rest/356/qejxunvz8s4bmtb2/"
#endif

// Update interval: 30 seconds (30000 ms)
unsigned long bitrix24UpdateInterval = 30000;

// Cached counts
static Bitrix24Counts cachedCounts = {0, 0, 0, 0, false, 0};

// Helper: Make HTTP GET request to Bitrix24 REST API
static String bitrix24Request(const char* method, const char* params = "") {
  if (WiFi.status() != WL_CONNECTED) {
    return "";
  }

  HTTPClient http;
  String url = String(BITRIX_HOSTNAME) + String(BITRIX_REST_ENDPOINT) + String(method);
  if (params && strlen(params) > 0) {
    url += "?";
    url += params;
  }

  http.begin(url);
  http.setTimeout(5000);

  int httpCode = http.GET();
  String payload;

  if (httpCode == 200) {
    payload = http.getString();
  }

  http.end();
  return payload;
}

// Initialize Bitrix24 client
void initBitrix24() {
  Serial.println("Bitrix24: Initialized");
  cachedCounts.valid = false;
  cachedCounts.lastUpdate = 0;
}

// Fetch unread dialogs count
// Strategy: Use im.counters.get → TYPE.DIALOG (number of dialogs with unread messages)
static bool fetchUnreadMessages(uint16_t* count) {
  if (!count) return false;

  String response = bitrix24Request("im.counters.get", "");
  if (response.length() == 0) {
    return false;
  }

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, response);
  response = "";

  if (err || !doc["result"].is<JsonObject>()) {
    return false;
  }

  JsonObject result = doc["result"].as<JsonObject>();

  // Debug: print TYPE values to inspect counters (ALL, CHAT, DIALOG, MESSENGER, etc.)
  if (result["TYPE"].is<JsonObject>()) {
    JsonObject type = result["TYPE"].as<JsonObject>();
    Serial.print("Bitrix24 TYPE: ");
    for (JsonPair kv : type) {
      Serial.print(kv.key().c_str());
      Serial.print("=");
      if (kv.value().is<int>()) {
        Serial.print(kv.value().as<int>());
      }
      Serial.print(" ");
    }
    Serial.println();
  }

  uint16_t unreadDialogs = 0;

  // Preferred: TYPE.DIALOG
  if (result["TYPE"].is<JsonObject>()) {
    JsonObject type = result["TYPE"].as<JsonObject>();
    if (type["DIALOG"].is<int>()) {
      unreadDialogs = type["DIALOG"].as<int>();
    }
  }

  // Fallback: direct DIALOG field
  if (unreadDialogs == 0 && result["DIALOG"].is<int>()) {
    unreadDialogs = result["DIALOG"].as<int>();
  }

  *count = unreadDialogs;

  // Compact log
  Serial.print("Bitrix24: Unread messages (dialogs): ");
  Serial.println(*count);

  return true;
}

// Fetch total unread messages (all types: dialogs + chats + groups)
// Returns TYPE.ALL or TYPE.MESSENGER
static bool fetchTotalUnreadMessages(uint16_t* count) {
  if (!count) return false;

  String response = bitrix24Request("im.counters.get", "");
  if (response.length() == 0) {
    return false;
  }

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, response);
  response = "";

  if (err || !doc["result"].is<JsonObject>()) {
    return false;
  }

  JsonObject result = doc["result"].as<JsonObject>();
  uint16_t totalUnread = 0;

  // Preferred: TYPE.ALL (total unread messages)
  if (result["TYPE"].is<JsonObject>()) {
    JsonObject type = result["TYPE"].as<JsonObject>();
    if (type["ALL"].is<int>()) {
      totalUnread = type["ALL"].as<int>();
    } else if (type["MESSENGER"].is<int>()) {
      totalUnread = type["MESSENGER"].as<int>();
    }
  }

  *count = totalUnread;
  return true;
}

// Fetch all notification counts from Bitrix24
bool fetchBitrix24Counts(Bitrix24Counts* counts) {
  if (!counts) return false;

  bool success = true;
  uint16_t unread = 0;
  uint16_t totalUnread = 0;
  uint16_t undone = 0;
  uint16_t groups = 0;

  // Fetch unread messages in dialogs
  if (!fetchUnreadMessages(&unread)) {
    success = false;
  }

  // Fetch total unread messages (all types)
  fetchTotalUnreadMessages(&totalUnread);

  // Tasks & groups are disabled for now
  undone = 0;
  groups = 0;

  counts->unreadMessages = unread;
  counts->totalUnreadMessages = totalUnread;
  counts->undoneTasks = undone;
  counts->groupsProjects = groups;
  counts->valid = success;
  counts->lastUpdate = millis();

  cachedCounts = *counts;

  Serial.print("Bitrix24: Dialogs: ");
  Serial.print(unread);
  Serial.print(", Total: ");
  Serial.println(totalUnread);

  return success;
}

// Get cached counts
Bitrix24Counts getBitrix24Counts() {
  return cachedCounts;
}

// Check if update is needed
bool shouldUpdateBitrix24() {
  if (!cachedCounts.valid) {
    return true;
  }

  unsigned long now = millis();
  if (now < cachedCounts.lastUpdate) {
    // millis() overflow, force refresh
    return true;
  }

  return (now - cachedCounts.lastUpdate) >= bitrix24UpdateInterval;
}

