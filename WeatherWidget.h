#ifndef WeatherWidget_h
#define WeatherWidget_h

#include "Globals.h"
#include "WeatherProvider.h"
#include "Bitmaps.h"

void displayWeatherInfo(WeatherProvider weatherProvider, TFT_eSPI &tft, bool reset);

#endif