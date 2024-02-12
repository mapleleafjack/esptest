// WiFiConnection.cpp
#include "WiFiConnection.h" // Include the corresponding header file
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Optionally, if ssid and password are used only within this implementation, define them here
const char *ssid = "YourSSID";         // Replace with your WiFi SSID
const char *password = "YourPassword"; // Replace with your WiFi password

void connectToWiFi()
{
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi Connected.");
}

void displayWiFiInfo()
{
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(0, 0);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.println("WiFi Connected");
    tft.print("IP: ");
    tft.println(WiFi.localIP());
}