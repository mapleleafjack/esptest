#ifndef WeatherProvider_h
#define WeatherProvider_h

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

class WeatherProvider {
public:
    WeatherProvider(const char* apiKey, float lat, float lon);
    void updateWeather();
    float getTemperature(int day) const; // Changed to get temperature of a specific day
    String getWeatherCondition(int day) const; // Changed to get weather condition of a specific day
    bool isDataAvailable() const;

private:
    const char* _apiKey;
    float _latitude;
    float _longitude;
    float _temperature[7]; // Changed to an array to store a week of temperatures
    String _weatherCondition[7]; // Changed to an array to store a week of weather conditions
    bool _dataAvailable;

    String constructApiUrl() const;
};

#endif