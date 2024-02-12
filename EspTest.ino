#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <WebServer.h>
#include <time.h>
#include "Globals.h"        // Include the header for the display object
#include "WiFiConnection.h" // Include the header for WiFi connectivity

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

enum DisplayMode
{
    DISPLAY_CIRCLE,
    DISPLAY_DATETIME
};

// Variable to track the current display mode
DisplayMode currentDisplayMode = DISPLAY_CIRCLE;

// ISR for handling button press
void IRAM_ATTR handleButtonPress()
{
    buttonPressed = true;
}

void displayRGBImage(const uint8_t *imageData, size_t length)
{
    tft.fillScreen(ST7735_BLACK); // Clear the screen first

    Serial.println("displaying image");

    int16_t x = 0, y = 0; // Start top-left
    for (size_t i = 0; i < length; i += 3)
    {
        // Extract RGB values
        uint8_t r = imageData[i];
        uint8_t g = imageData[i + 1];
        uint8_t b = imageData[i + 2];

        // Convert RGB888 to RGB565
        uint16_t color = tft.color565(r, g, b);

        Serial.println(color);

        // Draw pixel
        tft.drawPixel(x, y, color);

        // Move to next pixel
        x++;
        if (x >= 100)
        { // New line after 100 pixels
            x = 0;
            y++;
            if (y >= 100)
                break; // Stop if the end of the square is reached
        };
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Attach the interrupt to the button pin
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);

    tft.initR(INITR_GREENTAB);
    tft.fillScreen(ST7735_BLACK);
    logToTFT("LOADING....");

    connectToWiFi(); // Connect to WiFi

    // Configure NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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

    server.on("/image", HTTP_POST, []()
              {
    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "No data received");
      return;
    }
    const char* imageData = server.arg("plain").c_str();

    Serial.println("imageData");
    Serial.println(imageData);

    String message = server.arg("plain");  // Get the message
    Serial.println("message");
    Serial.println(message);

    size_t imageLength = server.arg("plain").length();
    displayRGBImage(reinterpret_cast<const uint8_t*>(imageData), 100);
    server.send(200, "text/plain", "Image displayed"); });

    // Handle not found
    server.onNotFound([]()
                      { server.send(404, "text/plain", "Not found"); });
}

void displayDateTimeLCDStyle(const char *date, const char *time)
{
    tft.fillScreen(ST7735_BLACK);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(2); // Set text size to larger scale for better visibility

    // Calculate appropriate starting positions for date and time
    int16_t xDate = (tft.width() - (6 * 6 * 2)) / 2; // Approximate centering
    int16_t yDate = (tft.height() / 2) - 16;         // Position for date
    int16_t xTime = (tft.width() - (6 * 6 * 2)) / 2; // Approximate centering
    int16_t yTime = (tft.height() / 2);              // Position for time, below date

    // Display date
    tft.setCursor(xDate, yDate);
    tft.println(date);

    // Display time
    tft.setCursor(xTime, yTime);
    tft.println(time);
}

void loop()
{
    switch (currentDisplayMode)
    {
    case DISPLAY_CIRCLE:
        animateCircle(); // Call your existing circle animation function
        break;
    case DISPLAY_DATETIME:
        updateDateTimeFromNTP();
        break;
    }

    if (buttonPressed)
    {
        buttonPressed = false;
        Serial.println("Button pressed!");
        if (currentDisplayMode == DISPLAY_CIRCLE)
        {
            currentDisplayMode = DISPLAY_DATETIME;
        }
        else
        {
            currentDisplayMode = DISPLAY_CIRCLE;
        }
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        connectToWiFi(); // Attempt to reconnect if disconnected
    }

    server.handleClient(); // Handle client requests

    delay(10); // Short delay to control animation speed and reduce CPU usage
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
    x++;
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

        if (updateDay)
        {
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
            // Update date part only
            tft.setTextColor(ST7735_WHITE, ST7735_BLACK); // Set text color and background to avoid erasing
            tft.setCursor(0, (tft.height() / 2) - 16);    // Adjust for your layout
            tft.println(dateStr);
        }

        // Print to console
        Serial.print("Updated time: ");
        Serial.print(dateStr);
        Serial.print(" ");
        Serial.println(timeStr);

        // Update previous timeinfo
        prevTimeinfo = timeinfo;
    }
}
