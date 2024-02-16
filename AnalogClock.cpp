#include "AnalogClock.h"

// Function to draw hour markers
void drawHourIndicators(TFT_eSPI& tft) {
  for (int i = 0; i < 12; ++i) {
    double angle = i * 30.0 * PI / 180.0; // Hour markers placed every 30 degrees

    int x1 = 80 + int(50 * cos(angle));
    int y1 = 64 + int(50 * sin(angle));
    int x2 = 80 + int(40 * cos(angle));
    int y2 = 64 + int(40 * sin(angle));
    tft.drawLine(x1, y1, x2, y2, ST7735_WHITE);
  }
}

void drawClockFace(TFT_eSPI& tft) {
  
  // Draw clock outline
  tft.fillCircle(80, 64, 62, ST7735_WHITE);
  tft.fillCircle(80, 64, 60, TFT_BLACK);

  // Draw hour markers
  drawHourIndicators(tft);
}


// Redraws a small segment of the clock face around a specific angle to minimize flickering
void redrawClockFaceSegment(TFT_eSPI& tft, double angle) {
  // Convert angle range to be positive and within 0 to 360 degrees
  angle = fmod(angle + 360.0, 360.0);

  // Determine which hour marker(s) might be affected by the angle
  int hourIndex = int(angle / 30.0);
  int prevHourIndex = (hourIndex + 11) % 12;
  int nextHourIndex = (hourIndex + 1) % 12;

  // Redraw the affected hour markers
  double angles[] = {hourIndex * 30.0, prevHourIndex * 30.0, nextHourIndex * 30.0};
  for (double a : angles) {
    double rad = a * PI / 180.0;
    int x1 = 80 + int(50 * cos(rad));
    int y1 = 64 + int(50 * sin(rad));
    int x2 = 80 + int(40 * cos(rad));
    int y2 = 64 + int(40 * sin(rad));
    tft.drawLine(x1, y1, x2, y2, ST7735_WHITE);
  }
}

// Function to calculate new X and Y coordinates for a hand
int calculateX(int length, double angle) {
  return 80 + int(length * cos(angle * PI / 180 - PI / 2));
}

int calculateY(int length, double angle) {
  return 64 + int(length * sin(angle * PI / 180 - PI / 2));
}


void eraseAndRedrawAffectedAreas(TFT_eSPI& tft, int lastX, int lastY, int length) {
  // Calculate the angle for the given coordinates
  double angleBefore = atan2(lastY - 64, lastX - 80) * 180.0 / PI;
  
  redrawClockFaceSegment(tft, angleBefore);
}


void updateHand(TFT_eSPI& tft, int &lastX, int &lastY, int newX, int newY, uint16_t color, int length) {
  // Erase the old hand position using the clock face background color
  tft.drawLine(80, 64, lastX, lastY, ST7735_BLACK); // Replace with actual background color

  // Erase the old hand position and redraw affected areas
  eraseAndRedrawAffectedAreas(tft, lastX, lastY, length);

  // Draw the new hand
  tft.drawLine(80, 64, newX, newY, color);

  // Update the last known positions
  lastX = newX;
  lastY = newY;
}

// Variables to keep track of the last positions of the hands
int lastHourX = 80, lastHourY = 64;
int lastMinuteX = 80, lastMinuteY = 64;
int lastSecondX = 80, lastSecondY = 64;

void updateAnalogClock(TimeProvider timeProvider, TFT_eSPI& tft, bool readraw) {
  static int lastSecond = -1;

  int currentSecond = timeProvider.getCurrentSecond();
  if (lastSecond != currentSecond) {
    lastSecond = currentSecond;

    if (readraw) {
      drawClockFace(tft);
    }

    int currentHour = timeProvider.getCurrentHour() % 12;
    int currentMinute = timeProvider.getCurrentMinute();
    
    // Calculate new positions for each hand
    double hourAngle = (timeProvider.getCurrentHour() % 12 + timeProvider.getCurrentMinute() / 60.0) * 30.0;
    double minuteAngle = timeProvider.getCurrentMinute() * 6.0;
    double secondAngle = currentSecond * 6.0;

    int newHourX = calculateX(20, hourAngle);
    int newHourY = calculateY(20, hourAngle);
    int newMinuteX = calculateX(30, minuteAngle);
    int newMinuteY = calculateY(30, minuteAngle);
    int newSecondX = calculateX(40, secondAngle);
    int newSecondY = calculateY(40, secondAngle);

    // Update hands, ensuring the previous positions are properly managed
    updateHand(tft, lastHourX, lastHourY, newHourX, newHourY, ST7735_RED, 20);
    updateHand(tft, lastMinuteX, lastMinuteY, newMinuteX, newMinuteY, ST7735_GREEN, 30);
    updateHand(tft, lastSecondX, lastSecondY, newSecondX, newSecondY, ST7735_BLUE, 40);
  }
}

