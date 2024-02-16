#include "TimeProvider.h"
#include <time.h>

TimeProvider::TimeProvider() : timeUpdated(false) {}

void TimeProvider::begin(const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec) {
    // Configure NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    this->updateTime(); // Initial time update to ensure it's set
}

bool TimeProvider::updateTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        this->timeUpdated = false;
        return false;
    }
    this->timeUpdated = true;
    return true;
}

String TimeProvider::getFormattedTime() {
    if (!this->timeUpdated) this->updateTime(); // Ensure time is updated
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeStr[16];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        return String(timeStr);
    }
    return "";
}

String TimeProvider::getFormattedDate() {
    if (!this->timeUpdated) this->updateTime(); // Ensure time is updated
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char dateStr[16];
        strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
        return String(dateStr);
    }
    return "";
}

int TimeProvider::getCurrentHour() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        return timeinfo.tm_hour;
    }
    return -1; // Return an invalid value if time could not be obtained
}

int TimeProvider::getCurrentMinute() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        return timeinfo.tm_min;
    }
    return -1;
}

int TimeProvider::getCurrentSecond() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        return timeinfo.tm_sec;
    }
    return -1;
}
