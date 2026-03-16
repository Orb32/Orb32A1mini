#include "app_context.h"
#include <time.h>

static void drawCenteredLine(const char *text, int y, int size, uint16_t color)
{
  tft.setTextSize(size);
  tft.setTextColor(color);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int x = (tft.width() - w) / 2;
  tft.setCursor(x, y);
  tft.print(text);
}

void drawClockTime(const String &timeStr, const String &dayStr, const String &monthYearStr)
{
  tft.fillRect(0, 60, tft.width(), 120, ST77XX_BLACK);

  drawCenteredLine(timeStr.c_str(), 70, 3, ST77XX_WHITE);
  drawCenteredLine(dayStr.c_str(), 120, 2, ST77XX_WHITE);
  drawCenteredLine(monthYearStr.c_str(), 150, 2, ST77XX_WHITE);
}

void drawClockView()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 5);
  tft.println("Clock");

  if (WiFi.status() != WL_CONNECTED)
  {
    drawCenteredText("WiFi not connected", 2);
    return;
  }

  String timeStr = "";
  String dayStr = "";
  String monthYearStr = "";

  if (!getClockStrings(timeStr, dayStr, monthYearStr))
  {
    drawCenteredText("Syncing time...", 2);
    return;
  }

  drawClockTime(timeStr, dayStr, monthYearStr);
}
