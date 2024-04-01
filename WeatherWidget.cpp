#include "WeatherWidget.h"

// Assuming these are defined somewhere globally or as static within the function
static float lastTemperatures[7] = {-999, -999, -999, -999, -999, -999, -999}; // Initialize with impossible values
static String lastWeatherConditions[7]; // Initialize with empty strings
static bool lastDataAvailable = false;

void displayWeatherInfo(WeatherProvider weatherProvider, TFT_eSPI &tft, bool reset) {
  // Definitions for display layout
  const int startX = 3;
  const int startY = 3;
  const int lineHeight = 18; // Adjust based on font size
  const char* dayNames[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

  bool isWeatherDataAvailable = weatherProvider.isDataAvailable();

  if (reset) {
    // On reset, clear the entire screen
    tft.fillScreen(ST7735_BLACK);
    for (int i = 0; i < 7; ++i) {
      lastTemperatures[i] = -999; // Reset to force update
      lastWeatherConditions[i] = ""; // Reset to force update
    }
    lastDataAvailable = false; // Force redraw after reset
  }

  for (int i = 0; i < 7; ++i) {
    int posY = startY + i * lineHeight; // Calculate Y position for each line
    float temp = isWeatherDataAvailable ? weatherProvider.getTemperature(i) : -999;
    String condition = isWeatherDataAvailable ? weatherProvider.getWeatherCondition(i) : "N/A";

    // Only update the line if the data has changed or if it's a reset
    if (temp != lastTemperatures[i] || condition != lastWeatherConditions[i] || !lastDataAvailable) {
      // Clear the line before redrawing
      tft.fillRect(startX, posY, 160 - startX, lineHeight - 1, ST7735_BLACK);

      // Redraw the line
      tft.setCursor(startX, posY);
      tft.print(dayNames[i]);
      tft.print(": ");
      if (temp != -999) { // Check if temperature data is valid
        tft.print(temp, 1); // Display temperature with one decimal place
        tft.print("C ");
        tft.print(condition); // Display the condition
      } else {
        tft.print("N/A");
      }

      // Update the last known values
      lastTemperatures[i] = temp;
      lastWeatherConditions[i] = condition;
    }
  }

  if (!isWeatherDataAvailable && lastDataAvailable) {
    // If data was available before but now isn't, clear the screen and show a message
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(startX, startY);
    tft.println("Weather data not available.");
    for (int i = 0; i < 7; ++i) {
      lastTemperatures[i] = -999; // Reset to ensure update when data is available again
      lastWeatherConditions[i] = ""; // Reset to ensure update
    }
  }

  lastDataAvailable = isWeatherDataAvailable; // Update the data availability status
}
