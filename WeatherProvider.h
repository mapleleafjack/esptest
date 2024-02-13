#ifndef WeatherProvider_h
#define WeatherProvider_h

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class WeatherProvider {
public:
    WeatherProvider(const char* apiKey, float lat, float lon);
    void updateWeather();
    float getTemperature() const;
    String getWeatherCondition() const;
    bool isDataAvailable() const;

private:
    const char* _apiKey;
    float _latitude;
    float _longitude;
    float _temperature;
    String _weatherCondition;
    bool _dataAvailable;

    String constructApiUrl() const;
};

#endif
