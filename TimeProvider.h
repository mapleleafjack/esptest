#ifndef TimeProvider_h
#define TimeProvider_h

#include <Arduino.h>

class TimeProvider {
public:
    TimeProvider();
    void begin(const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec);
    bool updateTime();
    String getFormattedTime();
    String getFormattedDate();
    int getCurrentHour();
    int getCurrentMinute();
    int getCurrentSecond();
private:
    bool timeUpdated;
};

#endif
