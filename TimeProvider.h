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
private:
    bool timeUpdated;
};

#endif
