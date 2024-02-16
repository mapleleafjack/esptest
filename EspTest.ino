#include <SPI.h>
#include <WebServer.h>
#include <time.h>
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "Globals.h"
#include "WiFiConnection.h"
#include "WeatherProvider.h"
#include "TimeProvider.h"
#include "Bitmaps.h"
#include "AnalogClock.h"

#define BUTTON_PIN 0

TFT_eSPI tft = TFT_eSPI();                         // Create display object
// Global variable to track button press
volatile bool buttonPressed = false;
volatile bool redraw_clock = true;

// Web server on port 80
WebServer server(80);

// Target FPS
const int targetFPS = 30;
// Calculate the time for each frame in milliseconds
const long frameTime = 1000 / targetFPS;

static unsigned long lastUpdateTime = 0;

enum DisplayMode {
  DISPLAY_DATETIME,
  DISPLAY_WEATHER_INFO
};

// Variable to track the current display mode
DisplayMode currentDisplayMode = DISPLAY_DATETIME;

TimeProvider timeProvider;
WeatherProvider weatherProvider("c32e13c5b20783f46b02e0f218fc39e9", 51.5074, -0.1278);

// ISR for handling button press
void IRAM_ATTR handleButtonPress() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Attach the interrupt to the button pin
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);

  logToTFT("Loading >.<");

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

static float lastTemperature = -999.0;  // Impossible initial value to ensure update occurs
static String lastWeatherCondition = "";

void displayWeatherInfo() {
  float globalTemperature = weatherProvider.getTemperature();
  String globalWeatherCondition = weatherProvider.getWeatherCondition();
  bool isWeatherDataAvailable = weatherProvider.isDataAvailable();

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

    tft.drawBitmap(50, 50, splitCloudsBitmap, 64, 64, ST7735_WHITE, ST7735_BLACK);
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
  String formattedTime = timeProvider.getFormattedTime();
  String formattedDate = timeProvider.getFormattedDate();

  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.println(formattedTime);

  tft.setCursor(0, 20);
  tft.println(formattedDate);
}

double degToRad(double degrees) {
  return degrees * (PI / 180.0);
}




void loop() {
  unsigned long frameStartTime = millis();

  switch (currentDisplayMode) {
    case DISPLAY_DATETIME:
      updateAnalogClock(timeProvider, tft, redraw_clock);
      redraw_clock = false;
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
    

    if (currentDisplayMode == DISPLAY_WEATHER_INFO) {
      currentDisplayMode = DISPLAY_DATETIME;
      redraw_clock = true;
    } else if (currentDisplayMode == DISPLAY_DATETIME) {
      currentDisplayMode = DISPLAY_WEATHER_INFO;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();  // Attempt to reconnect if disconnected
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
