// Bitrix24 API integration implementation

#include "bitrix24.h"
#include "wifi_telegram.h"
#include "storage.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>

#ifndef BITRIX_HOSTNAME
#define BITRIX_HOSTNAME "https://npfreom.bitrix24.ru"
#endif

#ifndef BITRIX_REST_ENDPOINT
#define BITRIX_REST_ENDPOINT "/rest/356/qejxunvz8s4bmtb2/"
#endif

// Credentials storage (loaded from NVS or build flags)
static char bitrixHostname[128] = "";
static char bitrixRestEndpoint[128] = "";

// Initialize Bitrix24 credentials from NVS or build flags
static void initBitrix24Credentials() {
  if (strlen(bitrixHostname) == 0) {  // Only load once
    // Try to load from NVS
    if (!loadBitrix24Credentials(bitrixHostname, sizeof(bitrixHostname), 
                                 bitrixRestEndpoint, sizeof(bitrixRestEndpoint))) {
      // Fall back to build flags
      strncpy(bitrixHostname, BITRIX_HOSTNAME, sizeof(bitrixHostname) - 1);
      strncpy(bitrixRestEndpoint, BITRIX_REST_ENDPOINT, sizeof(bitrixRestEndpoint) - 1);
    }
    
    // Clean up REST endpoint: remove any protocol/hostname if accidentally included
    // Use fast C string functions instead of String objects for better performance
    char* protocolPos = strstr(bitrixRestEndpoint, "://");
    if (protocolPos != NULL) {
      // Contains protocol, extract path part
      char* pathStart = strchr(protocolPos + 3, '/');
      if (pathStart != NULL) {
        // Move path to beginning of buffer
        size_t pathLen = strlen(pathStart);
        memmove(bitrixRestEndpoint, pathStart, pathLen + 1);
      } else {
        // No path found, assume it's just the endpoint
        if (bitrixRestEndpoint[0] != '/') {
          memmove(bitrixRestEndpoint + 1, bitrixRestEndpoint, strlen(bitrixRestEndpoint) + 1);
          bitrixRestEndpoint[0] = '/';
        }
      }
    }
    // Ensure it ends with / (fast check)
    size_t len = strlen(bitrixRestEndpoint);
    if (len > 0 && bitrixRestEndpoint[len - 1] != '/') {
      if (len < sizeof(bitrixRestEndpoint) - 1) {
        bitrixRestEndpoint[len] = '/';
        bitrixRestEndpoint[len + 1] = '\0';
      }
    }
  }
}

// Update interval: 30 seconds (30000 ms)
unsigned long bitrix24UpdateInterval = 30000;
// Minimum retry delay after a failed fetch: 30 seconds (prevents infinite loop, matches normal interval)
static const unsigned long BITRIX24_MIN_RETRY_DELAY = 30000;

// Cached counts
static Bitrix24Counts cachedCounts = {0, 0, 0, 0, 0, 0, 0, false, 0};
// Previous counts for change detection
static Bitrix24Counts previousCounts = {0, 0, 0, 0, 0, 0, 0, false, 0};

// Selected group id (0 = disabled)
static uint32_t selectedGroupId = 0;

// Current Bitrix24 user ID (determined at runtime via user.current)
static uint32_t bitrixCurrentUserId = 0;

// Cached Bitrix24 "today" date string (YYYY-MM-DD) from server.time
static String bitrixTodayDate;
static unsigned long bitrixTodayLastFetch = 0;

// Notification rate-limits and thresholds
static const unsigned long UNREAD_NOTIFY_RATE_MS = 5UL * 60UL * 1000UL; // 5 minutes
static const uint16_t UNREAD_NOTIFY_DELTA = 3; // require at least +3 change to notify outside immediate transitions
static unsigned long lastUnreadNotifyMs = 0;
static uint16_t lastUnreadNotifiedCount = 0;

static const unsigned long UNDONE_NOTIFY_RATE_MS = 10UL * 60UL * 1000UL; // 10 minutes
static const uint16_t UNDONE_NOTIFY_DELTA = 2;
static unsigned long lastUndoneNotifyMs = 0;
static uint16_t lastUndoneNotifiedCount = 0;

static const unsigned long EXPIRED_NOTIFY_RATE_MS = 15UL * 60UL * 1000UL; // 15 minutes
static const uint16_t EXPIRED_NOTIFY_DELTA = 1;
static unsigned long lastExpiredNotifyMs = 0;
static uint16_t lastExpiredNotifiedCount = 0;

// Helper: Make HTTP GET request to Bitrix24 REST API
static String bitrix24Request(const char* method, const char* params = "") {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Bitrix24: WiFi not connected, skipping ");
    Serial.println(method);
    return "";
  }

  // Ensure credentials are loaded
  initBitrix24Credentials();

  HTTPClient http;
  String url = String(bitrixHostname) + String(bitrixRestEndpoint) + String(method);
  if (params && strlen(params) > 0) {
    url += "?";
    url += params;
  }

  // Debug: Log URL structure for troubleshooting (mask endpoint secret)
  int endpointLen = strlen(bitrixRestEndpoint);
  Serial.print("Bitrix24: Calling ");
  Serial.print(method);
  Serial.print(" -> ");
  Serial.print(bitrixHostname);
  if (endpointLen > 15) {
    // Show first 8 chars and last 3 chars of endpoint
    Serial.print(String(bitrixRestEndpoint).substring(0, 8) + "...[masked]");
  } else {
    Serial.print("[endpoint]");
  }
  Serial.print(method);
  Serial.println();

  http.begin(url);
  http.setTimeout(5000);

  int httpCode = http.GET();
  String payload;

  if (httpCode == 200) {
    payload = http.getString();
  } else {
    Serial.print("Bitrix24 API error: ");
    Serial.print(method);
    Serial.print(" returned HTTP ");
    Serial.println(httpCode);
    // Show URL structure for debugging (mask endpoint secret)
    Serial.print("  URL format: ");
    Serial.print(bitrixHostname);
    if (endpointLen > 15) {
      Serial.print(String(bitrixRestEndpoint).substring(0, 8) + "...[masked]");
    } else {
      Serial.print("[endpoint]");
    }
    Serial.print(method);
    Serial.println();
    Serial.print("  Expected format: https://domain.bitrix24.ru/rest/USER_ID/WEBHOOK_CODE/method");
    Serial.println();
  }

  http.end();
  return payload;
}

// Initialize Bitrix24 client
void initBitrix24() {
  // Load credentials from NVS or build flags
  initBitrix24Credentials();
  
  Serial.println("Bitrix24: Initialized");
  // Don't log sensitive credentials
  cachedCounts.valid = false;
  cachedCounts.lastUpdate = 0;
  bitrixCurrentUserId = 0;
  selectedGroupId = 0;
}

// Reload Bitrix24 credentials from NVS
void reloadBitrix24Credentials() {
  // Clear existing credentials
  bitrixHostname[0] = '\0';
  bitrixRestEndpoint[0] = '\0';
  
  // Reload from NVS
  initBitrix24Credentials();
  
  // Invalidate cache to force refresh
  cachedCounts.valid = false;
}

void setBitrixSelectedGroupId(uint32_t groupId) {
  selectedGroupId = groupId;
  // Force refresh on next loop
  cachedCounts.valid = false;
}

uint32_t getBitrixSelectedGroupId() {
  return selectedGroupId;
}

// Force immediate Bitrix24 update (invalidates cache)
void forceBitrix24Update() {
  cachedCounts.valid = false;
  cachedCounts.lastUpdate = 0;  // Reset timestamp to allow immediate update
}

// Fetch current Bitrix24 user ID using user.current
// This avoids hardcoding USER_ID (e.g. 17) and always uses the webhook's user
static bool fetchCurrentUserId() {
  if (bitrixCurrentUserId != 0) {
    // Already known
    return true;
  }

  String response = bitrix24Request("user.current", "");
  if (response.length() == 0) {
    Serial.println("Bitrix24: fetchCurrentUserId - empty response");
    return false;
  }

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, response);
  response = "";

  if (err) {
    Serial.print("Bitrix24: fetchCurrentUserId - JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  if (!doc["result"].is<JsonObject>()) {
    Serial.println("Bitrix24: fetchCurrentUserId - result is not an object");
    return false;
  }

  JsonObject result = doc["result"].as<JsonObject>();

  // ID can be string or int; handle both
  uint32_t id = 0;
  if (result["ID"].is<const char*>()) {
    String idStr = result["ID"].as<const char*>();
    id = idStr.toInt();
  } else if (result["ID"].is<int>()) {
    id = result["ID"].as<int>();
  }

  if (id == 0) {
    return false;
  }

  bitrixCurrentUserId = id;

  return true;
}

// Fetch current Bitrix24 date (YYYY-MM-DD) using server.time
static bool fetchBitrixTodayDate(String *outDate) {
  if (!outDate) return false;

  // Refresh once per minute at most
  unsigned long now = millis();
  if (bitrixTodayDate.length() == 10 && (now - bitrixTodayLastFetch) < 60000UL) {
    *outDate = bitrixTodayDate;
    return true;
  }

  String response = bitrix24Request("server.time", "");
  if (response.length() == 0) {
    return false;
  }

  // Try to parse JSON response
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, response);
  response = "";

  if (err) {
    Serial.print("Bitrix24: server.time parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  // Check different possible response structures
  String dateStr;

  // Case 1: result is already a string datetime, e.g. "2026-01-19T15:58:59+03:00"
  if (doc["result"].is<const char*>()) {
    dateStr = doc["result"].as<const char*>();
  }

  // Case 2: result is an object with time/TIME field
  if (dateStr.length() == 0 && doc["result"].is<JsonObject>()) {
    JsonObject result = doc["result"].as<JsonObject>();
    if (result["time"].is<const char*>()) {
      dateStr = result["time"].as<const char*>();
    } else if (result["TIME"].is<const char*>()) {
      dateStr = result["TIME"].as<const char*>();
    }
  }
  
  // Case 3: direct time field at root
  if (dateStr.length() == 0 && doc["time"].is<const char*>()) {
    dateStr = doc["time"].as<const char*>();
  }

  if (dateStr.length() == 0) {
    Serial.println("Bitrix24: server.time has no time field in result");
    return false;
  }

  // Extract YYYY-MM-DD from the date string (might be "YYYY-MM-DD HH:MM:SS" or timestamp)
  // Look for YYYY-MM-DD pattern
  int dateStart = dateStr.indexOf("202");
  if (dateStart < 0) dateStart = dateStr.indexOf("2025");
  if (dateStart < 0) dateStart = dateStr.indexOf("2026");
  
  if (dateStart < 0 || dateStart + 10 > (int)dateStr.length()) {
    Serial.print("Bitrix24: server.time date format not recognized: ");
    Serial.println(dateStr);
    return false;
  }

  String dateOnly = dateStr.substring(dateStart, dateStart + 10); // YYYY-MM-DD
  bitrixTodayDate = dateOnly;
  bitrixTodayLastFetch = now;

  *outDate = dateOnly;
  return true;
}

static uint16_t parseTotalFromJson(const String& response) {
  uint16_t c = 0;
  int totalPos = response.indexOf("\"total\":");
  if (totalPos < 0) totalPos = response.indexOf("\"TOTAL\":");
  if (totalPos >= 0) {
    int numStart = response.indexOf(':', totalPos);
    if (numStart >= 0) {
      numStart++;
      while (numStart < (int)response.length() && response[numStart] == ' ') numStart++;
      int numEnd = numStart;
      while (numEnd < (int)response.length() && response[numEnd] >= '0' && response[numEnd] <= '9') numEnd++;
      if (numEnd > numStart) c = (uint16_t)response.substring(numStart, numEnd).toInt();
    }
  }
  return c;
}

// Fetch global "All your tasks" count (no specific group):
// - For current user as RESPONSIBLE ("Делаю")
// - "All your tasks" = active tasks (statuses 1,2,3,4,6) + delayed tasks
static bool fetchGlobalAllTasks(uint16_t* allTasks) {
  if (!allTasks) return false;
  *allTasks = 0;

  // Ensure current user ID
  if (!fetchCurrentUserId()) {
    return false;
  }

  // Get today's date for delayed filter
  String today;
  if (!fetchBitrixTodayDate(&today)) {
    return false;
  }

  // Delayed tasks: past deadline, responsible = current user, not completed
  uint16_t delayed = 0;
  {
    String delayedParams = "filter[!DEADLINE]="
                         + String("&filter[<DEADLINE]=") + today
                         + "&filter[RESPONSIBLE_ID]=" + String(bitrixCurrentUserId)
                         + "&filter[!STATUS]=5"
                         + "&nav_params[nPageSize]=1"
                         + "&nav_params[iNumPage]=1"
                         + "&select[]=ID";

    String resp = bitrix24Request("tasks.task.list", delayedParams.c_str());
    if (resp.length() > 0) {
      delayed = parseTotalFromJson(resp);
    }
  }

  // Active (non-delayed) tasks for this user, any group:
  // statuses: 1 (new), 2 (waiting), 3 (in progress), 4 (waiting for control), 6 (postponed)
  uint16_t active = 0;
  {
    String activeParams = "filter[RESPONSIBLE_ID]=" + String(bitrixCurrentUserId)
                        + "&filter[STATUS][]=1"
                        + "&filter[STATUS][]=2"
                        + "&filter[STATUS][]=3"
                        + "&filter[STATUS][]=4"
                        + "&filter[STATUS][]=6"
                        + "&nav_params[nPageSize]=1"
                        + "&nav_params[iNumPage]=1"
                        + "&select[]=ID";

    String resp = bitrix24Request("tasks.task.list", activeParams.c_str());
    if (resp.length() > 0) {
      active = parseTotalFromJson(resp);
    }
  }

  *allTasks = (uint16_t)(active + delayed);
  return true;
}

static bool fetchGroupDelayedAndComments(uint32_t groupId, uint16_t* delayed, uint16_t* comments) {
  if (!delayed || !comments) return false;
  *delayed = 0;
  *comments = 0;
  if (groupId == 0) return false;

  // Ensure we know current user; we'll count only tasks where this user is RESPONSIBLE ("Делаю")
  if (!fetchCurrentUserId()) {
    return false;
  }

  String today;
  if (!fetchBitrixTodayDate(&today)) {
    return false;
  }

  // Delayed tasks in group: past deadline, RESPONSIBLE_ID = current user, not completed
  String delayedParams = "filter[GROUP_ID]=" + String(groupId)
                       + "&filter[!DEADLINE]="
                       + "&filter[<DEADLINE]=" + today
                       + "&filter[RESPONSIBLE_ID]=" + String(bitrixCurrentUserId)
                       + "&filter[!STATUS]=5"
                       + "&nav_params[nPageSize]=1"
                       + "&nav_params[iNumPage]=1"
                       + "&select[]=ID";

  String resp1 = bitrix24Request("tasks.task.list", delayedParams.c_str());
  if (resp1.length() > 0) {
    *delayed = parseTotalFromJson(resp1);
    // Compact debug: just show filters and total
    Serial.println("Bitrix24 GroupStats Delayed: tasks.task.list params:");
    Serial.println(delayedParams);
  }

  // "All tasks" in group: tasks where current user is RESPONSIBLE ("Делаю")
  // and status is one of: new (1), waiting (2), in progress (3), waiting for control (4), postponed (6).
  // This includes delayed tasks (past deadline) as long as they have these statuses.
  // We explicitly don't filter by deadline to include all active tasks.
  String commentsParams = "filter[GROUP_ID]=" + String(groupId)
                        + "&filter[RESPONSIBLE_ID]=" + String(bitrixCurrentUserId)
                        + "&filter[STATUS][]=1"
                        + "&filter[STATUS][]=2"
                        + "&filter[STATUS][]=3"
                        + "&filter[STATUS][]=4"
                        + "&filter[STATUS][]=6"
                        + "&nav_params[nPageSize]=1"
                        + "&nav_params[iNumPage]=1"
                        + "&select[]=ID";

  String resp2 = bitrix24Request("tasks.task.list", commentsParams.c_str());
  if (resp2.length() > 0) {
    // Base count: all active tasks for this group & responsible user
    *comments = parseTotalFromJson(resp2);

    // The user expects "All your tasks" to be:
    //   non-delayed active tasks + delayed tasks
    // even if Bitrix doesn't include the delayed ones in the same filter.
    // Therefore we explicitly add the delayed count on top.
    if (*delayed > 0) {
      *comments = (uint16_t)(*comments + *delayed);
    }

    // Debug: show filters + list of IDs only (more readable)
    Serial.println("Bitrix24 GroupStats AllTasks: tasks.task.list params:");
    Serial.println(commentsParams);

    DynamicJsonDocument doc2(16384);
    DeserializationError err2 = deserializeJson(doc2, resp2);
    resp2 = "";
    if (!err2) {
      JsonArray tasks;
      if (doc2["result"].is<JsonArray>()) {
        tasks = doc2["result"].as<JsonArray>();
      } else if (doc2["result"].is<JsonObject>() && doc2["result"]["tasks"].is<JsonArray>()) {
        tasks = doc2["result"]["tasks"].as<JsonArray>();
      }
      if (!tasks.isNull()) {
        Serial.print("  AllTasks IDs (first 50): ");
        uint8_t printed = 0;
        for (JsonObject t : tasks) {
          if (printed > 0) Serial.print(", ");
          if (t["ID"].is<const char*>()) Serial.print(t["ID"].as<const char*>());
          else if (t["id"].is<const char*>()) Serial.print(t["id"].as<const char*>());
          else if (t["ID"].is<int>()) Serial.print(t["ID"].as<int>());
          else if (t["id"].is<int>()) Serial.print(t["id"].as<int>());
          else Serial.print("?");
          printed++;
          if (printed >= 50) break;
        }
        Serial.println();
      }
    } else {
      Serial.print("Bitrix24 GroupStats AllTasks parse error: ");
      Serial.println(err2.c_str());
    }
  }

  return true;
}

// Public wrapper used by Telegram code to get group stats on demand
bool bitrixGetGroupStats(uint32_t groupId, uint16_t* delayed, uint16_t* comments) {
  return fetchGroupDelayedAndComments(groupId, delayed, comments);
}

// Best-effort helper: get group/workgroup name by ID using sonet_group.get
String bitrixGetGroupName(uint32_t groupId) {
  if (groupId == 0) return "";

  // Use FILTER[ID] according to sonet_group.get docs
  String params = "FILTER[ID]=" + String(groupId);
  String resp = bitrix24Request("sonet_group.get", params.c_str());
  if (resp.length() == 0) return "";

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, resp);
  resp = "";
  if (err) return "";

  // result can be object or array[0]
  if (doc["result"].is<JsonObject>()) {
    JsonObject g = doc["result"].as<JsonObject>();
    if (g["NAME"].is<const char*>()) return String(g["NAME"].as<const char*>());
    if (g["name"].is<const char*>()) return String(g["name"].as<const char*>());
  }
  if (doc["result"].is<JsonArray>()) {
    JsonArray arr = doc["result"].as<JsonArray>();
    if (arr.size() > 0 && arr[0].is<JsonObject>()) {
      JsonObject g = arr[0].as<JsonObject>();
      if (g["NAME"].is<const char*>()) return String(g["NAME"].as<const char*>());
      if (g["name"].is<const char*>()) return String(g["name"].as<const char*>());
    }
  }
  return "";
}

// (Name-based helpers removed; we use IDs only)

// Fetch unread dialogs count
// Strategy: Use im.counters.get → TYPE.DIALOG (number of dialogs with unread messages)
static bool fetchUnreadMessages(uint16_t* count) {
  if (!count) return false;

  String response = bitrix24Request("im.counters.get", "");
  if (response.length() == 0) {
    Serial.println("Bitrix24: fetchUnreadMessages - empty response");
    return false;
  }

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, response);
  response = "";

  if (err) {
    Serial.print("Bitrix24: fetchUnreadMessages - JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }
  
  if (!doc["result"].is<JsonObject>()) {
    Serial.println("Bitrix24: fetchUnreadMessages - result is not an object");
    return false;
  }

  JsonObject result = doc["result"].as<JsonObject>();

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

// Fetch undone RPA / user process tasks count
// Strategy: Use bizproc.task.list (RPA user processes UI is built on top of bizproc)
static bool fetchUndoneTasks(uint16_t* count) {
  if (!count) return false;

  // Ensure we know the current Bitrix24 user ID (linked to this webhook)
  if (!fetchCurrentUserId()) {
    return false;
  }

  // Get tasks for the correct user ID
  // NOTE: This corresponds to RPA user tasks you see under /rpa/ → /bizproc/userprocesses/
  // We explicitly filter by USER_ID instead of hardcoding (e.g. 17).
  //
  // IMPORTANT: By default the API only returns a few fields (ENTITY, DOCUMENT_ID, ID, etc.).
  // We must explicitly ask for USER_ID and status fields so we can find tasks that are:
  //   - assigned to this user
  //   - still not completed (undone)
  //
  // Bitrix list syntax: FILTER[FIELD]=..., SELECT[]=FIELD1, SELECT[]=FIELD2, ...
  String params = "FILTER[USER_ID]=" + String(bitrixCurrentUserId)
                  + "&SELECT[]=ID"
                  + "&SELECT[]=USER_ID"
                  + "&SELECT[]=STATUS"
                  + "&SELECT[]=STATUS_ID"
                  + "&SELECT[]=STATUS_NAME";
  
  String response = bitrix24Request("bizproc.task.list", params.c_str());
  if (response.length() == 0) {
    return false;
  }

  // Calculate buffer size: response length + some overhead for JSON structure
  // Use at least 32KB to handle large responses (23KB+ seen in practice)
  size_t bufferSize = response.length() + 8192;  // Add 8KB overhead
  if (bufferSize < 32768) bufferSize = 32768;  // Minimum 32KB
  if (bufferSize > 65536) bufferSize = 65536;  // Maximum 64KB (ESP32-C6 limit)

  DynamicJsonDocument doc(bufferSize);
  DeserializationError err = deserializeJson(doc, response);
  response = "";  // Free memory immediately

  if (err) {
    return false;
  }

  uint16_t taskCount = 0;
  JsonArray tasks;

  // Try different response structures
  if (doc["result"].is<JsonArray>()) {
    tasks = doc["result"].as<JsonArray>();
  } else if (doc["result"].is<JsonObject>()) {
    JsonObject result = doc["result"].as<JsonObject>();
    
    // Check for "tasks" array
    if (result["tasks"].is<JsonArray>()) {
      tasks = result["tasks"].as<JsonArray>();
    }
    // Check for "total" count field
    else if (result["total"].is<int>()) {
      taskCount = result["total"].as<int>();
      *count = taskCount;
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }

  // Count only undone tasks that are actually assigned to current user
  // STATUS = 0 means undone/active
  for (JsonVariant task : tasks) {
    if (task.is<JsonObject>()) {
      JsonObject t = task.as<JsonObject>();
      
      // Check USER_ID matches current user
      bool isAssignedToUser = false;
      if (t["USER_ID"].is<int>()) {
        int userId = t["USER_ID"].as<int>();
        isAssignedToUser = (userId == (int)bitrixCurrentUserId);
      } else if (t["USER_ID"].is<const char*>()) {
        String userIdStr = t["USER_ID"].as<const char*>();
        uint32_t userId = userIdStr.toInt();
        isAssignedToUser = (userId == bitrixCurrentUserId);
      }
      
      // Check STATUS field: 0 = undone/active
      // Bitrix may return it as int OR as string, so handle both.
      bool isUndone = false;
      int status = -1;
      if (t["STATUS"].is<int>()) {
        status = t["STATUS"].as<int>();
      } else if (t["STATUS"].is<const char*>()) {
        String statusStr = t["STATUS"].as<const char*>();
        status = statusStr.toInt();
      }
      isUndone = (status == 0);  // STATUS=0 means undone/active
      
      // Count only if: assigned to user AND undone (STATUS=0)
      if (isAssignedToUser && isUndone) {
        taskCount++;
      }
    }
  }

  *count = taskCount;

  return true;
}

// Fetch expired (late) tasks/projects count
// Strategy: Use tasks.task.list with tiny page size and read result.total
// This keeps responses small and avoids NoMemory.
static bool fetchExpiredTasks(uint16_t* count) {
  if (!count) return false;

  // Ensure we know the current Bitrix24 user ID
  if (!fetchCurrentUserId()) {
    return false;
  }

  // Get current date from Bitrix (server.time) so it's not hardcoded
  String today;
  if (!fetchBitrixTodayDate(&today)) {
    *count = 0;
    return false;
  }

  // Filter: past deadline, RESPONSIBLE_ID = current user, !STATUS=5 (not completed)
  String params = "filter[RESPONSIBLE_ID]=" + String(bitrixCurrentUserId)
                + "&filter[!DEADLINE]="          // Has deadline (not empty)
                + "&filter[<DEADLINE]=" + today   // Deadline before today
                + "&filter[!STATUS]=5"            // Exclude completed tasks
                + "&nav_params[nPageSize]=1"
                + "&nav_params[iNumPage]=1"
                + "&select[]=ID";

  String response = bitrix24Request("tasks.task.list", params.c_str());
  if (response.length() == 0) {
    *count = 0;
    return false;
  }

  // Parse `"total":` from the raw JSON (avoid ArduinoJson NoMemory)
  uint16_t expiredCount = 0;
  int totalPos = response.indexOf("\"total\":");
  if (totalPos < 0) totalPos = response.indexOf("\"TOTAL\":");

  if (totalPos >= 0) {
    int numStart = response.indexOf(':', totalPos);
    if (numStart >= 0) {
      numStart++;
      while (numStart < (int)response.length() && response[numStart] == ' ') numStart++;
      int numEnd = numStart;
      while (numEnd < (int)response.length() && response[numEnd] >= '0' && response[numEnd] <= '9') {
        numEnd++;
      }
      if (numEnd > numStart) {
        expiredCount = (uint16_t)response.substring(numStart, numEnd).toInt();
      }
    }
  }

  response = "";
  *count = expiredCount;
  return true;
}

// Fetch total comments count
// Strategy: Try to get comments count from various sources
static bool fetchTotalComments(uint16_t* count) {
  if (!count) return false;

  // Disabled heavy API calls; we derive comments from total unread instead.
  *count = 0;
  return true;
}

// Check for changes and send Telegram notifications (status is edited in-place; alerts are rate-limited and may be queued)
static void checkAndNotifyChanges(const Bitrix24Counts& newCounts) {
  // Always update the single editable status message with current counts
  String status = "📌 Bitrix — 📨 ";
  status += String(newCounts.unreadMessages);
  status += " • 📋 ";
  status += String(newCounts.undoneTasks);
  status += " • ⏰ ";
  // Show group delayed if group mode active
  if (getBitrixSelectedGroupId() != 0) status += String(newCounts.groupDelayedTasks);
  else status += String(newCounts.expiredTasks);
  queueBitrixStatusUpdate(status);

  unsigned long now = millis();

  // Skip if previous counts are not valid (first run) — initialize rate-limit baseline
  if (!previousCounts.valid) {
    previousCounts = newCounts;
    lastUnreadNotifiedCount = newCounts.unreadMessages;
    lastUndoneNotifiedCount = newCounts.undoneTasks;
    lastExpiredNotifiedCount = (getBitrixSelectedGroupId() != 0) ? newCounts.groupDelayedTasks : newCounts.expiredTasks;
    lastUnreadNotifyMs = now;
    lastUndoneNotifyMs = now;
    lastExpiredNotifyMs = now;
    return;
  }

  // Helper: absolute difference
  auto absdiff = [](int a, int b)->int { return (a > b) ? (a - b) : (b - a); };

  // Unread messages: immediate on 0<>>0 transitions; otherwise notify when delta >= threshold AND rate allows
  if (previousCounts.unreadMessages != newCounts.unreadMessages) {
    bool zeroTransition = (previousCounts.unreadMessages == 0 && newCounts.unreadMessages > 0) ||
                          (previousCounts.unreadMessages > 0 && newCounts.unreadMessages == 0);
    int deltaSinceLastNotify = absdiff((int)newCounts.unreadMessages, (int)lastUnreadNotifiedCount);

    if (zeroTransition || (deltaSinceLastNotify >= (int)UNREAD_NOTIFY_DELTA && (now - lastUnreadNotifyMs) >= UNREAD_NOTIFY_RATE_MS)) {
      String msg = "📨 <b>Unread Messages:</b> " + String(newCounts.unreadMessages);
      int trend = (int)newCounts.unreadMessages - (int)previousCounts.unreadMessages;
      msg += (trend > 0) ? " ⬆️" : " ⬇️";
      queueBitrixAlert(msg);
      lastUnreadNotifiedCount = newCounts.unreadMessages;
      lastUnreadNotifyMs = now;
    } else {
      // Suppress immediate alert to avoid spam; still accumulate if user is focusing
      if (isWorkSession) {
        String msg = "📨 <b>Unread Messages (suppressed):</b> " + String(newCounts.unreadMessages);
        queueBitrixAlert(msg);
      }
    }
  }

  // Undone tasks: similar logic but with its thresholds
  if (previousCounts.undoneTasks != newCounts.undoneTasks) {
    bool zeroTransition = (previousCounts.undoneTasks == 0 && newCounts.undoneTasks > 0) ||
                          (previousCounts.undoneTasks > 0 && newCounts.undoneTasks == 0);
    int deltaSinceLastNotify = absdiff((int)newCounts.undoneTasks, (int)lastUndoneNotifiedCount);

    if (zeroTransition || (deltaSinceLastNotify >= (int)UNDONE_NOTIFY_DELTA && (now - lastUndoneNotifyMs) >= UNDONE_NOTIFY_RATE_MS)) {
      String msg = "📋 <b>Undone Tasks:</b> " + String(newCounts.undoneTasks);
      msg += (newCounts.undoneTasks > previousCounts.undoneTasks) ? " ⬆️" : " ⬇️";
      queueBitrixAlert(msg);
      lastUndoneNotifiedCount = newCounts.undoneTasks;
      lastUndoneNotifyMs = now;
    } else {
      if (isWorkSession) {
        String msg = "📋 <b>Undone Tasks (suppressed):</b> " + String(newCounts.undoneTasks);
        queueBitrixAlert(msg);
      }
    }
  }

  // Expired / Group delayed: handle group mode separately but with same rate-limit logic
  uint32_t currentGroupId = getBitrixSelectedGroupId();
  if (currentGroupId != 0) {
    if (previousCounts.groupDelayedTasks != newCounts.groupDelayedTasks) {
      int deltaSinceLastNotify = absdiff((int)newCounts.groupDelayedTasks, (int)lastExpiredNotifiedCount);
      bool zeroTransition = (previousCounts.groupDelayedTasks == 0 && newCounts.groupDelayedTasks > 0) ||
                            (previousCounts.groupDelayedTasks > 0 && newCounts.groupDelayedTasks == 0);
      if (zeroTransition || (deltaSinceLastNotify >= (int)EXPIRED_NOTIFY_DELTA && (now - lastExpiredNotifyMs) >= EXPIRED_NOTIFY_RATE_MS)) {
        String msg = "⏰ <b>Expired Tasks:</b> " + String(newCounts.groupDelayedTasks);
        msg += (newCounts.groupDelayedTasks > previousCounts.groupDelayedTasks) ? " ⬆️" : " ⬇️";
        String groupName = bitrixGetGroupName(currentGroupId);
        if (groupName.length() > 0) {
          msg += "\n📁 <b>Group:</b> ";
          msg += groupName;
        }
        queueBitrixAlert(msg);
        lastExpiredNotifiedCount = newCounts.groupDelayedTasks;
        lastExpiredNotifyMs = now;
      } else {
        if (isWorkSession) {
          String msg = "⏰ <b>Expired Tasks (suppressed):</b> " + String(newCounts.groupDelayedTasks);
          queueBitrixAlert(msg);
        }
      }
    }
  } else {
    if (previousCounts.expiredTasks != newCounts.expiredTasks) {
      int deltaSinceLastNotify = absdiff((int)newCounts.expiredTasks, (int)lastExpiredNotifiedCount);
      bool zeroTransition = (previousCounts.expiredTasks == 0 && newCounts.expiredTasks > 0) ||
                            (previousCounts.expiredTasks > 0 && newCounts.expiredTasks == 0);
      if (zeroTransition || (deltaSinceLastNotify >= (int)EXPIRED_NOTIFY_DELTA && (now - lastExpiredNotifyMs) >= EXPIRED_NOTIFY_RATE_MS)) {
        String msg = "⏰ <b>Expired Tasks:</b> " + String(newCounts.expiredTasks);
        msg += (newCounts.expiredTasks > previousCounts.expiredTasks) ? " ⬆️" : " ⬇️";
        queueBitrixAlert(msg);
        lastExpiredNotifiedCount = newCounts.expiredTasks;
        lastExpiredNotifyMs = now;
      } else {
        if (isWorkSession) {
          String msg = "⏰ <b>Expired Tasks (suppressed):</b> " + String(newCounts.expiredTasks);
          queueBitrixAlert(msg);
        }
      }
    }
  }

  // Update previous counts
  previousCounts = newCounts;
}

// Fetch all notification counts from Bitrix24
bool fetchBitrix24Counts(Bitrix24Counts* counts) {
  if (!counts) return false;

  bool success = true;
  uint16_t unread = 0;
  uint16_t totalUnread = 0;
  uint16_t undone = 0;
  uint16_t expired = 0;
  uint16_t comments = 0;
  uint16_t groupDelayed = 0;
  uint16_t groupComments = 0;

  // Fetch unread messages in dialogs
  if (!fetchUnreadMessages(&unread)) {
    success = false;
  }

  // Fetch total unread messages (all types)
  fetchTotalUnreadMessages(&totalUnread);

  // Fetch undone business process tasks
  fetchUndoneTasks(&undone);

  // Fetch expired (late) tasks/projects (count-only, low-memory)
  fetchExpiredTasks(&expired);

  // Third section logic:
  // - If a specific group is selected: use per-group stats (delayed + all your tasks)
  // - If no group is selected: show global "All your tasks" (across all groups)
  if (selectedGroupId != 0) {
    // Group mode
    fetchGroupDelayedAndComments(selectedGroupId, &groupDelayed, &groupComments);
    // Keep comments as total unread messages for backwards compatibility (not shown in UI in group mode)
    comments = totalUnread;
  } else {
    // Global mode: "All tasks" subtitle should show all active + delayed tasks for current user
    if (!fetchGlobalAllTasks(&comments)) {
      // Fallback: at least don't break, keep previous behavior based on unread messages
      comments = totalUnread;
    }
  }

  counts->unreadMessages = unread;
  counts->totalUnreadMessages = totalUnread;
  counts->undoneTasks = undone;
  counts->expiredTasks = expired;
  counts->totalComments = comments;
  counts->groupDelayedTasks = groupDelayed;
  counts->groupComments = groupComments;
  counts->valid = success;
  counts->lastUpdate = millis();

  // Always update cache timestamp (even on failure) to prevent infinite retry loop
  // The shouldUpdateBitrix24() function will enforce minimum retry delay
  cachedCounts = *counts;
  
  // Check for changes and send notifications
  if (success) {
    checkAndNotifyChanges(*counts);
  }
  
  // Only log detailed stats when WiFi is actually connected.
  // This prevents serial spam with zero values while WiFi is reconnecting
  // (for example, right after turning AP off and switching back to STA).
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Bitrix24: Dialogs: ");
    Serial.print(unread);
    Serial.print(", Total: ");
    Serial.print(totalUnread);
    Serial.print(", Tasks: ");
    Serial.print(undone);
    Serial.print(", Expired: ");
    Serial.print(expired);
    Serial.print(", Comments: ");
    Serial.print(comments);
    Serial.print(", GroupDelayed: ");
    Serial.print(groupDelayed);
    Serial.print(", All_your_group_tasks: ");
    Serial.println(groupComments);
  }

  return success;
}

// Get cached counts
Bitrix24Counts getBitrix24Counts() {
  return cachedCounts;
}

// Check if update is needed
bool shouldUpdateBitrix24() {
  // Don't update if WiFi is not connected (prevents infinite retry loop)
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  unsigned long now = millis();
  if (now < cachedCounts.lastUpdate) {
    // millis() overflow, force refresh
    return true;
  }
  
  unsigned long timeSinceLastUpdate = now - cachedCounts.lastUpdate;
  
  // If cache is invalid, only retry after minimum delay (prevents infinite loop)
  if (!cachedCounts.valid) {
    return timeSinceLastUpdate >= BITRIX24_MIN_RETRY_DELAY;
  }

  // Normal interval check for valid cache
  return timeSinceLastUpdate >= bitrix24UpdateInterval;
}

