#include "WeatherProvider.h"

WeatherProvider::WeatherProvider(const char* apiKey, float lat, float lon)
: _apiKey(apiKey), _latitude(lat), _longitude(lon), _dataAvailable(false) {
    // Initialize arrays to default values
    for(int i = 0; i < 7; ++i) {
        _temperature[i] = 0.0;
        _weatherCondition[i] = "";
    }
}

void WeatherProvider::updateWeather() {
    String apiUrl = constructApiUrl();
    HTTPClient http;
    http.begin(apiUrl);
    int httpCode = http.GET();

    if (httpCode > 0) {
        WiFiClient* stream = http.getStreamPtr();
        DynamicJsonDocument doc(8192); // Increased size to accommodate more data
        DeserializationError error = deserializeJson(doc, *stream);

        if (!error) {
            for(int i = 0; i < 7; ++i) { // Assuming the API provides a daily forecast array
                _temperature[i] = doc["list"][i]["main"]["temp"];
                _weatherCondition[i] = doc["list"][i]["weather"][0]["description"].as<String>();
            }
            _dataAvailable = true;
        } else {
            _dataAvailable = false;
        }
    } else {
        _dataAvailable = false;
    }

    http.end();
}

float WeatherProvider::getTemperature(int day) const {
    if(day >= 0 && day < 7) {
        return _temperature[day];
    } else {
        return 0.0; // Return a default value or handle error
    }
}

String WeatherProvider::getWeatherCondition(int day) const {
    if(day >= 0 && day < 7) {
        return _weatherCondition[day];
    } else {
        return ""; // Return a default value or handle error
    }
}

bool WeatherProvider::isDataAvailable() const {
    return _dataAvailable;
}

String WeatherProvider::constructApiUrl() const {
    String url = "https://api.openweathermap.org/data/2.5/forecast?lat=" + String(_latitude) + "&lon=" + String(_longitude) + "&units=metric&appid=" + _apiKey;
    return url;
}