#include "app_context.h"

void deleteSavedWiFi()
{
  // Disconnect from current WiFi
  WiFi.disconnect(true); // true = erase WiFi config from RAM
  WiFi.mode(WIFI_OFF);
  prefs.begin("wifi", false);
  prefs.remove("ssid");
  prefs.remove("pass");
  prefs.end();
}

// WiFi Status When boot
void showWiFiStatus(bool connected)
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(30, 120);

  if (connected)
    tft.println("WiFi is connected");
  else
  {
    tft.println("WiFi is not connected");
  }
}

// Check if WiFi credentials are saved
bool hasSavedWiFi()
{
  prefs.begin("wifi", true); // read-only
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  return (ssid.length() > 0 && pass.length() > 0);
}

// Auto connect to WiFi when boot
bool autoConnectWiFi()
{
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid == "")
    return false;

  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (millis() - start < 7000)
  {
    if (WiFi.status() == WL_CONNECTED)
      return true;
    delay(200);
  }
  return false;
}

// WIFI List Screen
void drawWiFiList()
{
  const int visibleCount = 7;
  const int startY = 40;
  const int rowH = 30;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 5);
  tft.print("WIFI SSID:");

  // Calculate scroll window
  int topIndex = 0;
  if (menuIndex >= visibleCount)
    topIndex = menuIndex - (visibleCount - 1);

  if (topIndex > wifiCount - visibleCount)
    topIndex = max(0, wifiCount - visibleCount);

  // Draw visible items
  for (int i = 0; i < visibleCount; i++)
  {
    int ssidIndex = topIndex + i;
    if (ssidIndex >= wifiCount)
      break;

    tft.setTextColor(ssidIndex == menuIndex ? ST77XX_BLUE : ST77XX_WHITE);
    tft.setCursor(10, startY + i * rowH);
    tft.println(ssids[ssidIndex]);
  }
}

// Password Screen
void drawKeyboard()
{
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setCursor(10, 5);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("WIFI: ");
  tft.println(ssids[selectedWiFi]);

  tft.setCursor(10, 35);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("PASSWORD: ");
  tft.println(password);

  // Upper Case indicator
  tft.setCursor(10, 60);
  tft.setTextColor(upperCase ? ST77XX_BLUE : DARKGREY);
  tft.print("ABC");
  tft.setTextColor(!upperCase ? ST77XX_BLUE : DARKGREY);
  tft.print("  abc");
  for (int y = 0; y < 3; y++)
  {
    for (int x = 0; x < 12; x++)
    {
      char c = keys[y][x];
      if (upperCase && c >= 'a' && c <= 'z')
        c -= 32;

      tft.setTextColor((x == keyX && y == keyY) ? ST77XX_BLUE : ST77XX_WHITE);
      tft.setCursor(10 + x * 22, 100 + y * 35);
      tft.print(c);
    }
  }
  tft.setTextSize(1.5);
  tft.setCursor(10, 220);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Upper Case: ^, Del: <, Connect: >");
}

// Wrong Password
void drawWiFiFail()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(30, 120);
  tft.println("Wrong Password!!");
}

// WiFi Connected Screen
void drawWiFiConnected()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(30, 120);
  tft.println("Connected to WiFi!");
}

// Scan WIFI
void scanWiFi()
{
  tft.fillScreen(ST77XX_BLACK);
  drawCenteredText("Scanning For WiFi...", 2);
  wifiCount = WiFi.scanNetworks();
  for (int i = 0; i < wifiCount && i < 15; i++)
    ssids[i] = WiFi.SSID(i);
  menuIndex = 0;
  drawWiFiList();
}
