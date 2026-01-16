// Bitrix24 API integration

#ifndef BITRIX24_H
#define BITRIX24_H

#include <Arduino.h>

// Bitrix24 notification counts
struct Bitrix24Counts {
  uint16_t unreadMessages;      // Unread messages count
  uint16_t undoneTasks;         // Undone automation & business process tasks
  uint16_t groupsProjects;      // Groups & projects notifications
  bool valid;                    // Whether data is valid
  unsigned long lastUpdate;     // Last update timestamp
};

// Initialize Bitrix24 client
void initBitrix24();

// Fetch all notification counts from Bitrix24
bool fetchBitrix24Counts(Bitrix24Counts* counts);

// Get cached counts (returns last fetched values)
Bitrix24Counts getBitrix24Counts();

// Check if update is needed (based on update interval)
bool shouldUpdateBitrix24();

// Update interval in milliseconds (default: 60000 = 1 minute)
extern unsigned long bitrix24UpdateInterval;

#endif // BITRIX24_H
