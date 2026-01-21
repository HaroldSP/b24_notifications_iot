// WiFi Access Point and Web Server functionality

#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <Arduino.h>

// Start Access Point mode and web server
void startAPMode();

// Stop Access Point mode and web server
void stopAPMode();

// Handle web server requests (call in loop)
void handleAPWebServer();

// Check if AP is currently active
bool isAPActive();

// Get AP SSID
const char* getAPSSID();

// Get AP password
const char* getAPPassword();

// Get AP IP address (returns empty string if not active)
String getAPIPAddress();

#endif // WIFI_AP_H
