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

// Current Bitrix24 user ID (determined at runtime via user.current)
static uint32_t bitrixCurrentUserId = 0;

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
  bitrixCurrentUserId = 0;
}

// Fetch current Bitrix24 user ID using user.current
// This avoids hardcoding USER_ID (e.g. 17) and always uses the webhook's user
static bool fetchCurrentUserId() {
  if (bitrixCurrentUserId != 0) {
    // Already known
    return true;
  }

  Serial.println("Bitrix24: Fetching current user via user.current...");
  String response = bitrix24Request("user.current", "");
  if (response.length() == 0) {
    Serial.println("Bitrix24: user.current returned empty response");
    return false;
  }

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, response);
  response = "";

  if (err) {
    Serial.print("Bitrix24: user.current parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  if (!doc["result"].is<JsonObject>()) {
    Serial.println("Bitrix24: user.current has no result object");
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
    Serial.println("Bitrix24: user.current ID is 0 or missing");
    return false;
  }

  bitrixCurrentUserId = id;
  Serial.print("Bitrix24: Current user ID detected: ");
  Serial.println(bitrixCurrentUserId);

  return true;
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

// Fetch undone RPA / user process tasks count
// Strategy: Use bizproc.task.list (RPA user processes UI is built on top of bizproc)
static bool fetchUndoneTasks(uint16_t* count) {
  if (!count) return false;

  // Ensure we know the current Bitrix24 user ID (linked to this webhook)
  if (!fetchCurrentUserId()) {
    Serial.println("Bitrix24 RPA: Cannot fetch current user ID, skipping tasks.");
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
  
  Serial.println("Bitrix24 RPA: Calling bizproc.task.list for user tasks...");
  Serial.print("Bitrix24 RPA: Request params: ");
  Serial.println(params);

  String response = bitrix24Request("bizproc.task.list", params.c_str());
  if (response.length() == 0) {
    Serial.println("Bitrix24 RPA: Empty response from bizproc.task.list");
    return false;
  }

  Serial.print("Bitrix24 RPA: Raw response length: ");
  Serial.println(response.length());

  // Calculate buffer size: response length + some overhead for JSON structure
  // Use at least 32KB to handle large responses (23KB+ seen in practice)
  size_t bufferSize = response.length() + 8192;  // Add 8KB overhead
  if (bufferSize < 32768) bufferSize = 32768;  // Minimum 32KB
  if (bufferSize > 65536) bufferSize = 65536;  // Maximum 64KB (ESP32-C6 limit)
  
  Serial.print("Bitrix24 RPA: Allocating JSON buffer: ");
  Serial.print(bufferSize);
  Serial.println(" bytes");

  DynamicJsonDocument doc(bufferSize);
  DeserializationError err = deserializeJson(doc, response);
  response = "";  // Free memory immediately

  if (err) {
    Serial.print("Bitrix24 RPA: Task parse error: ");
    Serial.println(err.c_str());
    Serial.print("Bitrix24 RPA: Buffer size was: ");
    Serial.println(bufferSize);
    return false;
  }

  // First, check root level for any count fields
  Serial.println("Bitrix24 RPA: Checking root level fields...");
  for (JsonPair kv : doc.as<JsonObject>()) {
    Serial.print("  Root[");
    Serial.print(kv.key().c_str());
    Serial.print("] = ");
    if (kv.value().is<int>()) {
      Serial.println(kv.value().as<int>());
    } else if (kv.value().is<JsonArray>()) {
      Serial.print("array[");
      Serial.print(kv.value().as<JsonArray>().size());
      Serial.println("]");
    } else if (kv.value().is<JsonObject>()) {
      Serial.println("object");
    } else {
      Serial.println("[other]");
    }
  }

  uint16_t taskCount = 0;
  JsonArray tasks;

  // Try different response structures
  if (doc["result"].is<JsonArray>()) {
    // Direct array of tasks
    tasks = doc["result"].as<JsonArray>();
    Serial.print("Bitrix24 RPA: result is array, total tasks = ");
    Serial.println(tasks.size());
  } else if (doc["result"].is<JsonObject>()) {
    JsonObject result = doc["result"].as<JsonObject>();
    
    Serial.println("Bitrix24 RPA: result is object, checking keys...");
    for (JsonPair kv : result) {
      Serial.print("  result[");
      Serial.print(kv.key().c_str());
      Serial.print("] = ");
      if (kv.value().is<int>()) {
        Serial.println(kv.value().as<int>());
        if (strcmp(kv.key().c_str(), "total") == 0 || 
            strcmp(kv.key().c_str(), "count") == 0 ||
            strcmp(kv.key().c_str(), "TOTAL") == 0) {
          taskCount = kv.value().as<int>();
          Serial.print("Bitrix24 RPA: Found count field: ");
          Serial.println(taskCount);
        }
      } else if (kv.value().is<JsonArray>()) {
        Serial.print("array[");
        Serial.print(kv.value().as<JsonArray>().size());
        Serial.println("]");
      } else {
        Serial.println("[other]");
      }
    }
    
    // Check for "tasks" array
    if (result["tasks"].is<JsonArray>()) {
      tasks = result["tasks"].as<JsonArray>();
      Serial.print("Bitrix24 RPA: result.tasks array, total tasks = ");
      Serial.println(tasks.size());
    }
    // Check for "total" count field
    else if (result["total"].is<int>()) {
      taskCount = result["total"].as<int>();
      Serial.print("Bitrix24 RPA: result.total field = ");
      Serial.println(taskCount);
      *count = taskCount;
      Serial.print("Bitrix24 RPA: Undone tasks (from total): ");
      Serial.println(*count);
      return true;
    } else if (taskCount > 0) {
      // We found a count field above
      *count = taskCount;
      Serial.print("Bitrix24 RPA: Undone tasks (from count field): ");
      Serial.println(*count);
      return true;
    } else {
      Serial.println("Bitrix24 RPA: result object has no tasks[] or count field.");
      return false;
    }
  } else {
    Serial.println("Bitrix24 RPA: Unexpected JSON structure (no result array or object).");
    return false;
  }

  // Inspect first 3 tasks to understand structure (one field per line, including nested objects)
  Serial.println("Bitrix24 RPA: Inspecting task structure (first 3 tasks, ALL fields including nested)...");
  int inspectCount = (tasks.size() < 3) ? tasks.size() : 3;
  for (int i = 0; i < inspectCount; i++) {
    if (tasks[i].is<JsonObject>()) {
      JsonObject task = tasks[i].as<JsonObject>();
      Serial.print("  Task[");
      Serial.print(i);
      Serial.println("] ALL fields (including nested):");
      for (JsonPair kv : task) {
        Serial.print("    ");
        Serial.print(kv.key().c_str());
        Serial.print(" = ");
        if (kv.value().is<int>()) {
          Serial.println(kv.value().as<int>());
        } else if (kv.value().is<bool>()) {
          Serial.println(kv.value().as<bool>() ? "true" : "false");
        } else if (kv.value().is<const char*>()) {
          String val = kv.value().as<const char*>();
          if (val.length() > 60) val = val.substring(0, 60) + "...";
          Serial.println(val.c_str());
        } else if (kv.value().is<JsonObject>()) {
          Serial.println("{object}");
          // Print nested object fields
          JsonObject nested = kv.value().as<JsonObject>();
          for (JsonPair nkv : nested) {
            Serial.print("      ");
            Serial.print(nkv.key().c_str());
            Serial.print(" = ");
            if (nkv.value().is<int>()) {
              Serial.println(nkv.value().as<int>());
            } else if (nkv.value().is<bool>()) {
              Serial.println(nkv.value().as<bool>() ? "true" : "false");
            } else if (nkv.value().is<const char*>()) {
              String val = nkv.value().as<const char*>();
              if (val.length() > 50) val = val.substring(0, 50) + "...";
              Serial.println(val.c_str());
            } else {
              Serial.println("[complex]");
            }
          }
        } else if (kv.value().is<JsonArray>()) {
          Serial.print("array[");
          Serial.print(kv.value().as<JsonArray>().size());
          Serial.println("]");
        } else {
          Serial.println("[complex]");
        }
      }
      Serial.println();
    }
  }

  // Count only undone tasks that are actually assigned to current user
  // Based on data analysis:
  //   - STATUS = 0 means undone/active (Task[2] with ID=20234 is what we need)
  //   - STATUS = 3 means done/completed (Task[0] with ID=20242 is NOT what we need)
  //   - All tasks have USER_ID = 17 (current user), so we filter by STATUS = 0
  Serial.print("Bitrix24 RPA: Filtering tasks - user ID=");
  Serial.print(bitrixCurrentUserId);
  Serial.println(", counting only STATUS=0 (undone)");
  
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
      
      // Check STATUS field: 0 = undone/active, 3 = done/completed
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
        Serial.print("Bitrix24 RPA: Counting undone task: ID=");
        // ID can also be string; print safely
        if (t["ID"].is<int>()) Serial.print(t["ID"].as<int>());
        else if (t["ID"].is<const char*>()) Serial.print(t["ID"].as<const char*>());
        else Serial.print("?");
        Serial.print(", STATUS=");
        Serial.println(status);
      }
    }
  }

  Serial.print("Bitrix24 RPA: Undone tasks (filtered): ");
  Serial.print(taskCount);
  Serial.print(" out of ");
  Serial.println(tasks.size());

  *count = taskCount;

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

  // Fetch undone business process tasks
  fetchUndoneTasks(&undone);

  // Groups are disabled for now
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
  Serial.print(totalUnread);
  Serial.print(", Tasks: ");
  Serial.println(undone);

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

