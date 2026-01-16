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
static Bitrix24Counts cachedCounts = {0, 0, 0, false, 0};

// Helper: Make HTTP GET request to Bitrix24 REST API
String bitrix24Request(const char* method, const char* params = "") {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Bitrix24: WiFi not connected");
    return "";
  }

  HTTPClient http;
  String url = String(BITRIX_HOSTNAME) + String(BITRIX_REST_ENDPOINT) + String(method);
  if (strlen(params) > 0) {
    url += "?" + String(params);
  }

  Serial.print("Bitrix24: Requesting ");
  Serial.println(url);

  http.begin(url);
  http.setTimeout(10000); // 10 second timeout
  
  int httpCode = http.GET();
  String payload = "";

  if (httpCode > 0) {
    payload = http.getString();
    Serial.print("Bitrix24: Response code ");
    Serial.println(httpCode);
  } else {
    Serial.print("Bitrix24: Request failed, error: ");
    Serial.println(httpCode);
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

// Fetch unread messages count
bool fetchUnreadMessages(uint16_t* count) {
  String response = bitrix24Request("im.counters.get");
  if (response.length() == 0) {
    return false;
  }

  // Increase buffer size for larger responses
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.print("Bitrix24: JSON parse error for im.counters.get: ");
    Serial.println(error.c_str());
    return false;
  }

  // Get TYPE.DIALOG counter (only messenger dialogs, not task chats)
  if (doc["result"]["TYPE"]["DIALOG"].is<int>()) {
    *count = doc["result"]["TYPE"]["DIALOG"].as<int>();
    return true;
  }

  return false;
}

// Fetch undone automation & business process tasks
bool fetchUndoneTasks(uint16_t* count) {
  // Get business process tasks for current user only
  // Using START=0 to get first page, we only need the total count
  String response = bitrix24Request("bizproc.task.list", "START=0");
  if (response.length() == 0) {
    return false;
  }

  // Increase buffer size for large responses (~19KB)
  // Use larger buffer to handle full response
  DynamicJsonDocument doc(24576); // 24KB buffer for large task lists
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.print("Bitrix24: JSON parse error for bizproc.task.list: ");
    Serial.println(error.c_str());
    Serial.print("Response length: ");
    Serial.println(response.length());
    if (response.length() > 0 && response.length() < 200) {
      Serial.print("Response preview: ");
      Serial.println(response.substring(0, min(200, (int)response.length())));
    }
    return false;
  }

  // Get total count
  if (doc["result"]["total"].is<int>()) {
    *count = doc["result"]["total"].as<int>();
    return true;
  }

  return false;
}

// Fetch groups & projects count
// For now, we'll use sonet_group.get to get total groups/projects
// You may want to add activity stream notifications later
bool fetchGroupsProjects(uint16_t* count) {
  // Limit to first page to reduce response size
  String response = bitrix24Request("sonet_group.get", "START=0");
  if (response.length() == 0) {
    return false;
  }

  // Increase buffer size for larger responses
  DynamicJsonDocument doc(12288); // 12KB buffer
  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.print("Bitrix24: JSON parse error for sonet_group.get: ");
    Serial.println(error.c_str());
    Serial.print("Response length: ");
    Serial.println(response.length());
    if (response.length() > 0 && response.length() < 200) {
      Serial.print("Response preview: ");
      Serial.println(response.substring(0, min(200, (int)response.length())));
    }
    return false;
  }

  // Get total count of groups/projects
  if (doc["result"]["total"].is<int>()) {
    *count = doc["result"]["total"].as<int>();
    return true;
  }

  return false;
}

// Fetch all notification counts from Bitrix24
bool fetchBitrix24Counts(Bitrix24Counts* counts) {
  if (counts == nullptr) {
    return false;
  }

  bool success = true;
  uint16_t unread = 0;
  uint16_t undone = 0;
  uint16_t groups = 0;

  // Fetch unread messages
  if (!fetchUnreadMessages(&unread)) {
    Serial.println("Bitrix24: Failed to fetch unread messages");
    success = false;
  } else {
    Serial.print("Bitrix24: Successfully fetched unread messages: ");
    Serial.println(unread);
  }

  // TODO: Re-enable when ready
  // Fetch undone tasks - DISABLED for now
  // if (!fetchUndoneTasks(&undone)) {
  //   Serial.println("Bitrix24: Failed to fetch undone tasks");
  //   success = false;
  // }
  undone = 0; // Placeholder

  // TODO: Re-enable when ready
  // Fetch groups/projects - DISABLED for now
  // if (!fetchGroupsProjects(&groups)) {
  //   Serial.println("Bitrix24: Failed to fetch groups/projects");
  //   success = false;
  // }
  groups = 0; // Placeholder

  // Update counts
  counts->unreadMessages = unread;
  counts->undoneTasks = undone;
  counts->groupsProjects = groups;
  counts->valid = success;
  counts->lastUpdate = millis();

  // Update cache
  cachedCounts = *counts;

  Serial.print("Bitrix24: Counts - Messages: ");
  Serial.print(unread);
  Serial.print(", Tasks: ");
  Serial.print(undone);
  Serial.print(", Groups: ");
  Serial.println(groups);

  return success;
}

// Get cached counts
Bitrix24Counts getBitrix24Counts() {
  return cachedCounts;
}

// Check if update is needed
bool shouldUpdateBitrix24() {
  if (!cachedCounts.valid) {
    return true; // Always update if no valid data
  }

  unsigned long now = millis();
  // Handle millis() overflow
  if (now < cachedCounts.lastUpdate) {
    return true; // Overflow occurred, update
  }

  return (now - cachedCounts.lastUpdate) >= bitrix24UpdateInterval;
}
