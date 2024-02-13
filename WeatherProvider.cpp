#include "WeatherProvider.h"

WeatherProvider::WeatherProvider(const char* apiKey, float lat, float lon)
: _apiKey(apiKey), _latitude(lat), _longitude(lon), _temperature(0.0), _weatherCondition(""), _dataAvailable(false) {}

void WeatherProvider::updateWeather() {
    String apiUrl = constructApiUrl();
    HTTPClient http;
    http.begin(apiUrl);
    int httpCode = http.GET();

    if (httpCode > 0) {
        WiFiClient* stream = http.getStreamPtr();
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, *stream);

        if (!error) {
            _temperature = doc["main"]["temp"];
            _weatherCondition = doc["weather"][0]["description"].as<String>();
            _dataAvailable = true;
        } else {
            _dataAvailable = false;
        }
    } else {
        _dataAvailable = false;
    }

    http.end();
}

float WeatherProvider::getTemperature() const {
    return _temperature;
}

String WeatherProvider::getWeatherCondition() const {
    return _weatherCondition;
}

bool WeatherProvider::isDataAvailable() const {
    return _dataAvailable;
}

String WeatherProvider::constructApiUrl() const {
    String url = "https://api.openweathermap.org/data/2.5/weather?lat=" + String(_latitude) + "&lon=" + String(_longitude) + "&units=metric&appid=" + _apiKey;
    return url;
}
