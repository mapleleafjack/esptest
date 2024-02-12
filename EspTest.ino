#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WebServer.h>
#include <time.h>
#include <Arduino.h>
#include "Globals.h"       
#include "WiFiConnection.h" 
#include <HTTPClient.h>
#include <ArduinoJson.h>

// NTP Servers:
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;        // Adjust according to your timezone in seconds
const int daylightOffset_sec = 3600; // Adjust if your country uses daylight saving time

// Pin definitions for the ESP32-CAM and ST7735
#define TFT_CS 13   // Chip select pin for TFT
#define TFT_RST 16  // Reset pin for TFT (can be connected to ESP32-CAM reset)
#define TFT_DC 2    // Data/command pin for TFT
#define TFT_MOSI 15 // Data out (MOSI) pin for SPI
#define TFT_SCLK 14 // Clock pin for SPI

#define BUTTON_PIN 0 // Define the button pin

// Create the display object
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Global variable to track button press
volatile bool buttonPressed = false;

const char *fixedDate = "2024-02-01";
const char *fixedTime = "12:34";

// Web server on port 80
WebServer server(80);

// Target FPS
const int targetFPS = 30;
// Calculate the time for each frame in milliseconds
const long frameTime = 1000 / targetFPS;



const char* weatherApiKey = "c32e13c5b20783f46b02e0f218fc39e9"; // Replace with your actual API key
// Example for London, UK. Replace with the desired coordinates
const float latitude = 51.5074;
const float longitude = -0.1278;

String weatherApiUrl = "https://api.openweathermap.org/data/2.5/forecast?lat=" + String(latitude) + "&lon=" + String(longitude) + "&units=metric&appid=" + weatherApiKey;

float globalTemperature;
String globalWeatherCondition;
bool isWeatherDataAvailable = false;


enum DisplayMode
{
    DISPLAY_CIRCLE,
    DISPLAY_DATETIME,
    DISPLAY_WIFI_INFO
};

// Variable to track the current display mode
DisplayMode currentDisplayMode = DISPLAY_CIRCLE;

// ISR for handling button press
void IRAM_ATTR handleButtonPress()
{
    buttonPressed = true;
}

void setup()
{
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Attach the interrupt to the button pin
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);

    tft.initR(INITR_GREENTAB);
    tft.setRotation(1); // rotate 90 degrees
    tft.fillScreen(ST7735_BLACK);
    logToTFT("Loading stuff, please be patient! >.<");

    connectToWiFi(); // Connect to WiFi

    // Configure NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    configureServer(); // Configure the server

    logToTFT("Ready!");
}

void configureServer()
{
    // Start the server
    server.begin();

    // Define endpoint for POST request
    server.on("/postMessage", HTTP_POST, []()
              {
    if (server.hasArg("plain") == false) {  // Check if body received
      server.send(400, "text/plain", "No message received");
      return;
    }
    String message = server.arg("plain");  // Get the message
    logToTFT(message);                     // Display on TFT
    server.send(200, "text/plain", "Message received: " + message); });

    // Handle not found
    server.onNotFound([]()
                      { server.send(404, "text/plain", "Not found"); });
}

void displayDateTimeLCDStyle(const char *date, const char *time)
{
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(2); // Set text size to larger scale for better visibility

    // Calculate appropriate starting positions for date and time
    // int16_t xDate = (tft.width() - (6 * 6 * 2)) / 2; // Approximate centering
    // int16_t yDate = (tft.height() / 2) - 16;         // Position for date
    // int16_t xTime = (tft.width() - (6 * 6 * 2)) / 2; // Approximate centering
    // int16_t yTime = (tft.height() / 2);              // Position for time, below date

    // Display date
    tft.setCursor(0, 0);
    tft.println(date);

    // Display time
    tft.setCursor(0, 1);
    tft.println(time);

    
}


void fetchAndDisplayWeather() {
  HTTPClient http;
  Serial.println(weatherApiUrl);
  
  http.begin(weatherApiUrl);
  int httpCode = http.GET();

  if (httpCode > 0) {
    const size_t capacity = JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(6) + 270;
    DynamicJsonDocument doc(capacity);

    String payload = http.getString();

    Serial.println(payload);
    deserializeJson(doc, payload);

    // Extract weather data and update global variables
    globalTemperature = 66.6;
    globalWeatherCondition = "Kinda meh";
    isWeatherDataAvailable = true;
  } else {
    Serial.println("Error on HTTP request");
    isWeatherDataAvailable = false;
  }

  http.end();
}

// Global variables to track the previously displayed values
float prevGlobalTemperature = -999; // Initialize with an impossible value
String prevGlobalWeatherCondition = "";
bool prevIsWeatherDataAvailable = false;

void displayWeatherInfo() {
    // Check if weather data availability has changed
    if (prevIsWeatherDataAvailable != isWeatherDataAvailable) {
        tft.fillScreen(ST7735_BLACK); // Clear the screen only if availability status changes
        prevIsWeatherDataAvailable = isWeatherDataAvailable;
    }
  
    if (isWeatherDataAvailable) {
        // Update temperature if it has changed
        if (prevGlobalTemperature != globalTemperature) {
            // Clear the temperature area
            tft.fillRect(3, 0, 80, 16, ST7735_BLACK); // Adjust the size as needed
            tft.setCursor(3, 0);
            tft.print("Temp: ");
            tft.println(globalTemperature);
            prevGlobalTemperature = globalTemperature;
        }

        // Update condition if it has changed
        if (prevGlobalWeatherCondition != globalWeatherCondition) {
            // Clear the condition area
            tft.fillRect(3, 16, 160, 16, ST7735_BLACK); // Adjust the size as needed
            tft.setCursor(3, 16);
            tft.print("Condition: ");
            tft.println(globalWeatherCondition);
            prevGlobalWeatherCondition = globalWeatherCondition;
        }
    } else {
        if (prevIsWeatherDataAvailable) { // Only clear and display this message if the availability status changed
            tft.setCursor(3, 0);
            tft.println("Weather data not available.");
        }
    }
}


void displayWiFiInfo()
{
    tft.setCursor(0, 0);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.println("WiFi Connected");
    tft.print("IP: ");
    tft.println(WiFi.localIP());
}

void animateCircle()
{
    static int x = 0; // Static variables retain their value between function calls
    int y = tft.height() / 2;
    int radius = 5;
    uint16_t color = ST7735_RED;

    // Clear only the area where the circle will be
    tft.fillRect(x - radius - 1, y - radius - 1, radius * 2 + 2, radius * 2 + 2, ST7735_BLACK);

    // Draw the circle at the new position
    tft.drawCircle(x, y, radius, color);

    // Update x for next position
    x += 1;
    if (x >= tft.width())
    {
        x = 0; // Reset x to start from the left again
    }
}

void logToTFT(String message)
{
    tft.fillScreen(ST7735_BLACK); // Clear the screen
    tft.setCursor(0, 0);          // Reset cursor to top left
    tft.println(message);         // Print the message
}

void updateDateTimeFromNTP()
{
    static struct tm prevTimeinfo; // Stores the previous time to compare changes
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }

    // Check what has changed
    bool updateHour = prevTimeinfo.tm_hour != timeinfo.tm_hour;
    bool updateMinute = prevTimeinfo.tm_min != timeinfo.tm_min;
    bool updateSecond = prevTimeinfo.tm_sec != timeinfo.tm_sec;
    bool updateDay = prevTimeinfo.tm_mday != timeinfo.tm_mday;

    // Only update the screen if there are changes
    if (updateHour || updateMinute || updateSecond || updateDay)
    {
        char timeStr[16];
        char dateStr[16];

        // Assuming you only want to update the time every second
        if (updateSecond)
        {
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
            // Update time part only
            tft.setTextColor(ST7735_WHITE, ST7735_BLACK); // Set text color and background to avoid erasing
            tft.setCursor(0, (tft.height() / 2));         // Adjust as necessary
            tft.println(timeStr);
        }

        if (updateSecond)
        {
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
            // Update date part only
            tft.setTextColor(ST7735_WHITE, ST7735_BLACK); // Set text color and background to avoid erasing
            tft.setCursor(0, (tft.height() / 2) - 16);    // Adjust for your layout
            tft.println(dateStr);
        }
        
        prevTimeinfo = timeinfo;
    }
}

void loop()
{
    unsigned long frameStartTime = millis();

    // Weather update logic moved outside of the DISPLAY_WIFI_INFO case
    static unsigned long lastUpdateTime = 0;
    if (millis() - lastUpdateTime > 60000) { // Check for weather update every minute
        fetchAndDisplayWeather(); // Updates the weather data
        lastUpdateTime = millis(); // Reset the timer for weather updates
    }

    switch (currentDisplayMode)
    {
    case DISPLAY_CIRCLE:
        animateCircle(); // Existing circle animation function
        break;
    case DISPLAY_DATETIME:
        updateDateTimeFromNTP();
        break;
    case DISPLAY_WIFI_INFO:
        // Display updated weather info without fetching new data here
        // Assuming the display function checks global weather variables
        displayWeatherInfo(); // Make sure this function exists to display weather info
        break;
    }

    // Existing button press handling and mode switching logic
    if (buttonPressed)
    {
        buttonPressed = false;
        tft.fillScreen(ST7735_BLACK); // Clear the screen
        
        Serial.println("Button pressed!");

        if (currentDisplayMode == DISPLAY_CIRCLE)
        {
            currentDisplayMode = DISPLAY_DATETIME;
        }
        else if (currentDisplayMode == DISPLAY_DATETIME)
        {
            currentDisplayMode = DISPLAY_WIFI_INFO; // Cycle to display WiFi info
        }
        else
        {
            currentDisplayMode = DISPLAY_CIRCLE; // Reset to initial display mode
        }
    }

    // Existing WiFi reconnection logic
    if (WiFi.status() != WL_CONNECTED) {
        connectToWiFi(); // Attempt to reconnect if disconnected
    }

    // Existing server client handling logic
    server.handleClient();

    // Existing frame timing logic
    unsigned long elapsedTime = millis() - frameStartTime;
    long timeToNextFrame = frameTime - elapsedTime;
    if (timeToNextFrame > 0) {
        delay(timeToNextFrame);
    }
}
