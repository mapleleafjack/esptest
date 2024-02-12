#ifndef WiFiConnection_h
#define WiFiConnection_h

#include "Globals.h"
#include <WiFi.h> // Include the WiFi library here, so it's available to the implementation

// Declaration of functions
void connectToWiFi();

// If needed, externalize constants (like SSID and password) only if they are used across multiple files
extern const char *ssid;
extern const char *password;

#endif