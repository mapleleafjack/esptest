#include "WeatherWidget.h"

void displayWeatherInfo(WeatherProvider weatherProvider, TFT_eSPI &tft, bool reset) {
  static float lastTemperature = -999.0;  // Impossible initial value to ensure update occurs
  static String lastWeatherCondition = "";

  if (reset) {
    lastTemperature = -999.0;   // Impossible initial value to ensure update occurs
    lastWeatherCondition = "";  // Impossible initial value to ensure update occurs
  }

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
