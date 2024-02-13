#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WebServer.h>
#include <time.h>
#include <Arduino.h>
#include "Globals.h"
#include "WiFiConnection.h"
#include "WeatherService.h"

// NTP Servers:
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;         // Adjust according to your timezone in seconds
const int daylightOffset_sec = 3600;  // Adjust if your country uses daylight saving time

// Pin definitions for the ESP32-CAM and ST7735
#define TFT_CS 13    // Chip select pin for TFT
#define TFT_RST 16   // Reset pin for TFT (can be connected to ESP32-CAM reset)
#define TFT_DC 2     // Data/command pin for TFT
#define TFT_MOSI 15  // Data out (MOSI) pin for SPI
#define TFT_SCLK 14  // Clock pin for SPI

#define BUTTON_PIN 0  // Define the button pin

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

static unsigned long lastUpdateTime = 0;

enum DisplayMode {
  DISPLAY_CIRCLE,
  DISPLAY_DATETIME,
  DISPLAY_WEATHER_INFO
};

// Variable to track the current display mode
DisplayMode currentDisplayMode = DISPLAY_CIRCLE;

// '1530369_weather_cloud_clouds_cloudy_icon', 64x64px
const unsigned char heartBitmap [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1f, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xc0, 0x00, 0x00, 
	0x00, 0x00, 0x0f, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xe0, 0x00, 0x00, 
	0x00, 0x00, 0x7f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 
	0x00, 0x01, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 
	0x00, 0x03, 0xff, 0xff, 0xf8, 0x0f, 0x80, 0x00, 0x00, 0x03, 0xff, 0xff, 0xe0, 0x07, 0x80, 0x00, 
	0x00, 0x03, 0xff, 0xff, 0xc3, 0xc3, 0xc0, 0x00, 0x00, 0x03, 0xff, 0xf8, 0x0f, 0xf1, 0xc0, 0x00, 
	0x00, 0x01, 0xff, 0xe0, 0x1f, 0xf8, 0xc0, 0x00, 0x00, 0x01, 0xff, 0xc3, 0xff, 0xfc, 0x00, 0x00, 
	0x00, 0x01, 0xff, 0xcf, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0xff, 0x8f, 0xff, 0xfe, 0x00, 0x00, 
	0x00, 0x00, 0x7f, 0x9f, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x1f, 0x9f, 0xff, 0xff, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


// ISR for handling button press
void IRAM_ATTR handleButtonPress() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Attach the interrupt to the button pin
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);

  tft.initR(INITR_GREENTAB);
  tft.setRotation(1);  // rotate 90 degrees
  tft.fillScreen(ST7735_BLACK);
  logToTFT("Loading stuff, please be patient! >.<");

  connectToWiFi();  // Connect to WiFi

  // Configure NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  configureServer();  // Configure the server

  logToTFT("Ready!");
}

void configureServer() {
  // Start the server
  server.begin();

  // Define endpoint for POST request
  server.on("/postMessage", HTTP_POST, []() {
    if (server.hasArg("plain") == false) {  // Check if body received
      server.send(400, "text/plain", "No message received");
      return;
    }
    String message = server.arg("plain");  // Get the message
    logToTFT(message);                     // Display on TFT
    server.send(200, "text/plain", "Message received: " + message);
  });

  // Handle not found
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
}

void displayDateTimeLCDStyle(const char *date, const char *time) {
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);  // Set text size to larger scale for better visibility

  // Display date
  tft.setCursor(0, 0);
  tft.println(date);

  // Display time
  tft.setCursor(0, 1);
  tft.println(time);
}



static float lastTemperature = -999.0;  // Impossible initial value to ensure update occurs
static String lastWeatherCondition = "";

void displayWeatherInfo() {

  if (globalTemperature != lastTemperature) {
    tft.fillRect(3, 0, 80, 16, ST7735_BLACK); 
    tft.setCursor(3, 0);
    tft.print("Temp: ");
    tft.println(globalTemperature, 1);  // Assuming you want one decimal place for temperature
    lastTemperature = globalTemperature;
  }

  if (globalWeatherCondition != lastWeatherCondition) {
    tft.fillRect(3, 16, 160, 16, ST7735_BLACK); 
    tft.setCursor(3, 16);
    tft.print("Condition: ");
    tft.println(globalWeatherCondition);
    lastWeatherCondition = globalWeatherCondition;

    tft.drawBitmap(50, 50, heartBitmap, 64, 64, ST7735_WHITE, ST7735_BLACK);
  }

  if (!isWeatherDataAvailable && lastTemperature != -999) { 
    tft.fillScreen(ST7735_BLACK); 
    tft.setCursor(3, 0);
    tft.println("Weather data not available.");
    // Reset stored values to force update when data becomes available again
    lastTemperature = -999;
    lastWeatherCondition = "";
  }
}


void displayWiFiInfo() {
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.println("WiFi Connected");
  tft.print("IP: ");
  tft.println(WiFi.localIP());
}

void animateCircle() {
  static int x = 0;  // Static variables retain their value between function calls
  int y = tft.height() / 2;
  int radius = 5;
  uint16_t color = ST7735_RED;

  // Clear only the area where the circle will be
  tft.fillRect(x - radius - 1, y - radius - 1, radius * 2 + 2, radius * 2 + 2, ST7735_BLACK);

  // Draw the circle at the new position
  tft.drawCircle(x, y, radius, color);

  // Update x for next position
  x += 1;
  if (x >= tft.width()) {
    x = 0;  // Reset x to start from the left again
  }
}

void logToTFT(String message) {
  tft.fillScreen(ST7735_BLACK);  // Clear the screen
  tft.setCursor(0, 0);           // Reset cursor to top left
  tft.println(message);          // Print the message
}

void updateDateTimeFromNTP() {
  static struct tm prevTimeinfo;  // Stores the previous time to compare changes
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  // Check what has changed
  bool updateHour = prevTimeinfo.tm_hour != timeinfo.tm_hour;
  bool updateMinute = prevTimeinfo.tm_min != timeinfo.tm_min;
  bool updateSecond = prevTimeinfo.tm_sec != timeinfo.tm_sec;
  bool updateDay = prevTimeinfo.tm_mday != timeinfo.tm_mday;

  // Only update the screen if there are changes
  if (updateHour || updateMinute || updateSecond || updateDay) {
    char timeStr[16];
    char dateStr[16];

    // Assuming you only want to update the time every second
    if (updateSecond) {
      strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
      // Set cursor position for time
      tft.setCursor(0, 0); // Adjust as necessary for your display layout
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);  // Set text color and background to avoid erasing
      tft.println(timeStr);
    }

    // Assuming the condition should be `updateDay` instead of `updateSecond` for updating the date
    if (updateSecond) {
      strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
      // Set cursor position for date, adjust y-coordinate as necessary
      tft.setCursor(0, 20); // This is just an example, adjust the Y coordinate according to your display and font size
      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);  // Set text color and background to avoid erasing
      tft.println(dateStr);
    }

    prevTimeinfo = timeinfo;
  }
}


void loop() {
  unsigned long frameStartTime = millis();

  switch (currentDisplayMode) {
    case DISPLAY_CIRCLE:
      animateCircle();
      break;
    case DISPLAY_DATETIME:
      updateDateTimeFromNTP();
      break;
    case DISPLAY_WEATHER_INFO:
      displayWeatherInfo(); 
      break;
  }

  if (buttonPressed) {
    Serial.println("Button pressed!");
    tft.fillScreen(ST7735_BLACK);  // Clear the screen

    buttonPressed = false;
    
    lastTemperature = -999.0;   // Impossible initial value to ensure update occurs
    lastWeatherCondition = "";  // Impossible initial value to ensure update occurs

    if (currentDisplayMode == DISPLAY_CIRCLE) {
      currentDisplayMode = DISPLAY_DATETIME;
    } else if (currentDisplayMode == DISPLAY_DATETIME) {
      currentDisplayMode = DISPLAY_WEATHER_INFO;  
    } else {
      currentDisplayMode = DISPLAY_CIRCLE; 
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();  // Attempt to reconnect if disconnected
  }

  if (lastUpdateTime == 0 || millis() - lastUpdateTime > 60000) {  // Check for weather update every minute
    fetchWeatherData();                                      
    lastUpdateTime = millis(); 
  }

  server.handleClient();

  unsigned long elapsedTime = millis() - frameStartTime;
  long timeToNextFrame = frameTime - elapsedTime;
  if (timeToNextFrame > 0) {
    delay(timeToNextFrame);
  }
}
