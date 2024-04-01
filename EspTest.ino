#include <SPI.h>
#include <WebServer.h>
#include <time.h>
#include <Arduino.h>
#include <Bounce2.h>



#include "Globals.h"
#include "WiFiConnection.h"
#include "WeatherProvider.h"
#include "TimeProvider.h"
#include "Bitmaps.h"
#include "AnalogClock.h"
#include "Animation.h"
#include "WeatherWidget.h"

#include <TFT_eSPI.h>

#define BUTTON_PIN 0

Bounce debouncer = Bounce();

TFT_eSPI tft = TFT_eSPI();  // Create display object

volatile bool redraw_clock = true;
volatile bool resetWeatherWidget = false;

// Web server on port 80
WebServer server(80);

// Target FPS
const int targetFPS = 30;
// Calculate the time for each frame in milliseconds
const long frameTime = 1000 / targetFPS;


long lastEncoderPosition = -999;

static unsigned long lastUpdateTime = 0;

enum DisplayMode {
  DISPLAY_DATETIME,
  DISPLAY_WEATHER_INFO
};

// Variable to track the current display mode
DisplayMode currentDisplayMode = DISPLAY_DATETIME;

TimeProvider timeProvider;
WeatherProvider weatherProvider("c32e13c5b20783f46b02e0f218fc39e9", 51.5074, -0.1278);

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);

  logToTFT("Loading >.<");

  // Initialize Bounce2 for the button
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(50);  // Set debounce interval to 50ms for reliability

  connectToWiFi();  // Connect to WiFi

  // Initialize TimeProvider with NTP settings
  timeProvider.begin("pool.ntp.org", 0, 3600);

  configureServer();  // Configure the server

  tft.fillScreen(ST7735_BLACK);  // Clear the screen
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

void logToTFT(String message) {
  tft.fillScreen(ST7735_BLACK);  // Clear the screen
  tft.setCursor(0, 0);           // Reset cursor to top left
  tft.println(message);          // Print the message
}


void loop() {
  unsigned long frameStartTime = millis();

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();  // Attempt to reconnect if disconnected
  }

  // Update the debouncer
  debouncer.update();

  switch (currentDisplayMode) {
    case DISPLAY_DATETIME:
      updateAnalogClock(timeProvider, tft, redraw_clock);
      redraw_clock = false;
      break;
    case DISPLAY_WEATHER_INFO:
      displayWeatherInfo(weatherProvider, tft, resetWeatherWidget);
      resetWeatherWidget = false;
      break;
  }

  if (debouncer.fell()) {
    Serial.println("Button pressed!");
    tft.fillScreen(ST7735_BLACK);  // Clear the screen

    if (currentDisplayMode == DISPLAY_WEATHER_INFO) {
      currentDisplayMode = DISPLAY_DATETIME;
      redraw_clock = true;
    } else if (currentDisplayMode == DISPLAY_DATETIME) {
      resetWeatherWidget = true;
      currentDisplayMode = DISPLAY_WEATHER_INFO;
    }
  }

  if (lastUpdateTime == 0 || millis() - lastUpdateTime > 60000) {  // Check for weather update every minute
    weatherProvider.updateWeather();                               // Fetch new weather data
    lastUpdateTime = millis();
  }

  server.handleClient();

  unsigned long elapsedTime = millis() - frameStartTime;
  long timeToNextFrame = frameTime - elapsedTime;
  if (timeToNextFrame > 0) {
    delay(timeToNextFrame);
  }
}
