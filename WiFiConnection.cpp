// WiFiConnection.cpp
#include "WiFiConnection.h" // Include the corresponding header file
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Optionally, if ssid and password are used only within this implementation, define them here
const char *ssid = "Hide your WIFI";         // Replace with your WiFi SSID
const char *password = "cowscowscows"; // Replace with your WiFi password

void connectToWiFi()
{
    Serial.println("Connecting to WIFI...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi Connected.");
}