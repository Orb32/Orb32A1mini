#include "app_context.h"

// Settings Screen
void drawSettings()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 5);
  tft.println("SETTINGS");

  tft.setCursor(270, 220);
  tft.println("v1.0");
  for (int i = 0; i < 5; i++)
  {
    tft.setTextColor(i == settingsIndex ? ST77XX_BLUE : ST77XX_WHITE);
    tft.setCursor(20, 50 + i * 35);
    tft.println(settingsMenu[i]);
  }
}
