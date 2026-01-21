// Bitrix24 API integration

#ifndef BITRIX24_H
#define BITRIX24_H

#include <Arduino.h>

// Bitrix24 notification counts
struct Bitrix24Counts {
  uint16_t unreadMessages;       // Unread messages in dialogs (TYPE.DIALOG)
  uint16_t totalUnreadMessages;  // Total unread messages (TYPE.ALL or TYPE.MESSENGER)
  uint16_t undoneTasks;          // Undone RPA user tasks (from RPA / user processes)
  uint16_t expiredTasks;         // Expired (late) tasks/projects
  uint16_t totalComments;        // Total comments count (for subtitle)
  uint16_t groupDelayedTasks;    // Overdue tasks in selected group (when group mode enabled)
  uint16_t groupComments;        // Tasks with new comments in selected group (best-effort)
  bool valid;                    // Whether data is valid
  unsigned long lastUpdate;      // Last update timestamp
};

// Initialize Bitrix24 client
void initBitrix24();

// Fetch all notification counts from Bitrix24
bool fetchBitrix24Counts(Bitrix24Counts* counts);

// Get cached counts (returns last fetched values)
Bitrix24Counts getBitrix24Counts();

// Selected group mode: show counters for a single workgroup/project
void setBitrixSelectedGroupId(uint32_t groupId);   // 0 disables group mode
uint32_t getBitrixSelectedGroupId();

// Optional helpers for selected group (used for Telegram messages)
// - Get group name by ID (may return empty string if not available)
String bitrixGetGroupName(uint32_t groupId);
// - Get delayed tasks and total count for a group (lightweight, count-only)
bool bitrixGetGroupStats(uint32_t groupId, uint16_t* delayed, uint16_t* comments);

// Check if update is needed (based on update interval)
bool shouldUpdateBitrix24();

// Reload Bitrix24 credentials from NVS (call after saving via web interface)
void reloadBitrix24Credentials();

// Update interval in milliseconds (default: 60000 = 1 minute)
extern unsigned long bitrix24UpdateInterval;

#endif // BITRIX24_H
