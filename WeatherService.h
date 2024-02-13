#ifndef WeatherService_h
#define WeatherService_h

#include <HTTPClient.h>
#include <ArduinoJson.h>

// Declare all necessary variables and functions here
extern float globalTemperature;
extern String globalWeatherCondition;
extern bool isWeatherDataAvailable;

void fetchWeatherData();

#endif
