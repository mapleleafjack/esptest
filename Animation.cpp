#include "Animation.h"

void animateCircle(TFT_eSPI& tft, uint16_t color) {
  static int x = 0;  // Static variables retain their value between function calls
  int y = tft.height() / 2;
  int radius = 5;

  // Clear only the area where the circle will be
  tft.fillRect(x - radius - 1, y - radius - 1, radius * 2 + 2, radius * 2 + 2, ST7735_BLACK);

  // Draw the circle at the new position
  tft.drawCircle(x, y, radius, color);

  // Update x for next position
  x += 1;
  if (x >= tft.width()) {
    x = 0;  // Reset x to start from the left again
  }
}
