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

#define BUTTON_PIN 0

TFT_eSPI tft = TFT_eSPI();                         // Create display object
// Global variable to track button press
volatile bool buttonPressed = false;

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




// Function to draw clock face with hour indicators
void drawClockFace() {
  
  // Draw clock outline
  tft.fillCircle(80, 64, 62, ST7735_WHITE);
  tft.fillCircle(80, 64, 60, TFT_BLACK);

  // Draw hour markers
  drawHourIndicators();
}

// Function to draw hour markers
void drawHourIndicators() {
  for (int i = 0; i < 12; ++i) {
    double angle = i * 30.0 * PI / 180.0; // Hour markers placed every 30 degrees
    int x1 = 80 + int(50 * cos(angle));
    int y1 = 64 + int(50 * sin(angle));
    int x2 = 80 + int(40 * cos(angle));
    int y2 = 64 + int(40 * sin(angle));
    tft.drawLine(x1, y1, x2, y2, ST7735_WHITE);
  }
}

// Redraws a small segment of the clock face around a specific angle to minimize flickering
void redrawClockFaceSegment(double angle) {
  // Convert angle range to be positive and within 0 to 360 degrees
  angle = fmod(angle + 360.0, 360.0);

  // Determine which hour marker(s) might be affected by the angle
  int hourIndex = int(angle / 30.0);
  int prevHourIndex = (hourIndex + 11) % 12;
  int nextHourIndex = (hourIndex + 1) % 12;

  // Redraw the affected hour markers
  double angles[] = {hourIndex * 30.0, prevHourIndex * 30.0, nextHourIndex * 30.0};
  for (double a : angles) {
    double rad = a * PI / 180.0;
    int x1 = 80 + int(50 * cos(rad));
    int y1 = 64 + int(50 * sin(rad));
    int x2 = 80 + int(40 * cos(rad));
    int y2 = 64 + int(40 * sin(rad));
    tft.drawLine(x1, y1, x2, y2, ST7735_WHITE);
  }
}

// Function to calculate new X and Y coordinates for a hand
int calculateX(int length, double angle) {
  return 80 + int(length * cos(angle * PI / 180 - PI / 2));
}

int calculateY(int length, double angle) {
  return 64 + int(length * sin(angle * PI / 180 - PI / 2));
}

// Enhanced erase and redraw function that handles all hands
void eraseAndRedrawAffectedAreas(int lastX, int lastY, int length) {
  // Calculate the angle for the given coordinates
  double angleBefore = atan2(lastY - 64, lastX - 80) * 180.0 / PI;
  
  // Redraw the affected clock face segment
  redrawClockFaceSegment(angleBefore);

  // If needed, adjust this to also account for the length of the hand
  // This could involve redrawing a larger segment of the clock face or more specific elements
}

// Update any hand by providing its new coordinates and color
void updateHand(int &lastX, int &lastY, int newX, int newY, uint16_t color, int length) {
      // Erase the old hand position using the clock face background color
  tft.drawLine(80, 64, lastX, lastY, ST7735_BLACK); // Replace with actual background color

  // Erase the old hand position and redraw affected areas
  eraseAndRedrawAffectedAreas(lastX, lastY, length);

  // Draw the new hand
  tft.drawLine(80, 64, newX, newY, color);

  // Update the last known positions
  lastX = newX;
  lastY = newY;
}

// Variables to keep track of the last positions of the hands
int lastHourX = 80, lastHourY = 64;
int lastMinuteX = 80, lastMinuteY = 64;
int lastSecondX = 80, lastSecondY = 64;

static bool clockFaceDrawn = false;

void updateAnalogClock() {
  static int lastSecond = -1;

  int currentSecond = timeProvider.getCurrentSecond();
  if (lastSecond != currentSecond) {
    lastSecond = currentSecond;

    if (!clockFaceDrawn) {
      drawClockFace();
      clockFaceDrawn = true;
    }

    int currentHour = timeProvider.getCurrentHour() % 12;
    int currentMinute = timeProvider.getCurrentMinute();
    
 // Calculate new positions for each hand
    double hourAngle = (timeProvider.getCurrentHour() % 12 + timeProvider.getCurrentMinute() / 60.0) * 30.0;
    double minuteAngle = timeProvider.getCurrentMinute() * 6.0;
    double secondAngle = currentSecond * 6.0;

    int newHourX = calculateX(20, hourAngle);
    int newHourY = calculateY(20, hourAngle);
    int newMinuteX = calculateX(30, minuteAngle);
    int newMinuteY = calculateY(30, minuteAngle);
    int newSecondX = calculateX(40, secondAngle);
    int newSecondY = calculateY(40, secondAngle);

    // Update hands, ensuring the previous positions are properly managed
    updateHand(lastHourX, lastHourY, newHourX, newHourY, ST7735_RED, 20);
    updateHand(lastMinuteX, lastMinuteY, newMinuteX, newMinuteY, ST7735_GREEN, 30);
    updateHand(lastSecondX, lastSecondY, newSecondX, newSecondY, ST7735_BLUE, 40);
  }
}




void loop() {
  unsigned long frameStartTime = millis();

  switch (currentDisplayMode) {
    case DISPLAY_DATETIME:
      updateAnalogClock();
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
    clockFaceDrawn = false;

    if (currentDisplayMode == DISPLAY_WEATHER_INFO) {
      currentDisplayMode = DISPLAY_DATETIME;
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
