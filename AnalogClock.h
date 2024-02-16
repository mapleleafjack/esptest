#ifndef AnalogClock_h
#define AnalogClock_h

#include <Arduino.h>
#include "TimeProvider.h"
#include <TFT_eSPI.h>

void updateAnalogClock(TimeProvider timeProvider, TFT_eSPI& tft, bool redraw);

#endif