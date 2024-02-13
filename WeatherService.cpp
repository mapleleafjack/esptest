#include "WeatherService.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// Initialization of global variables
float globalTemperature;
String globalWeatherCondition;
bool isWeatherDataAvailable = false;

// Your API Key
const char *weatherApiKey = "c32e13c5b20783f46b02e0f218fc39e9";
// Location coordinates
const float latitude = 51.5074;
const float longitude = -0.1278;
// API URL construction
String weatherApiUrl = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(latitude) + "&lon=" + String(longitude) + "&units=metric&appid=" + weatherApiKey;

void fetchWeatherData() {
  HTTPClient http;
  Serial.println(weatherApiUrl);

  http.begin(weatherApiUrl);
  int httpCode = http.GET();

  if (httpCode > 0) {
    WiFiClient *stream = http.getStreamPtr();

    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(6) + 270;
    DynamicJsonDocument doc(capacity);

    DeserializationError error = deserializeJson(doc, *stream);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      isWeatherDataAvailable = false;
    } else {

      float temperature = doc["main"]["temp"];                     // Temperature
      String weatherCondition = doc["weather"][0]["description"];  // Weather condition description
      int visibility = doc["visibility"];                          // Visibility
      float windSpeed = doc["wind"]["speed"];                      // Wind speed

      globalTemperature = temperature;
      globalWeatherCondition = weatherCondition;

      Serial.print("Temperature: ");
      Serial.println(globalTemperature);
      Serial.print("Weather Condition: ");
      Serial.println(globalWeatherCondition);

      isWeatherDataAvailable = true;
    }
  } else {
    Serial.println("Error on HTTP request");
    isWeatherDataAvailable = false;
  }

  http.end();
}