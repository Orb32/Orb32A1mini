#include "app_context.h"
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>

// -------- OTA --------
#define FW_VERSION "v1.0"
static const char *firmwareURL =
    "https://raw.githubusercontent.com/orb32/Orb32A1mini/main/firmware1.bin";
static const char *firmwareName = "Orb32 A1 mini v1.0";

static bool otaInProgress = false;
static bool firmwareAvailable = false;

static void drawStatusMessage(const char *line1, const char *line2, uint16_t color)
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(color);
  tft.setCursor(15, 70);
  tft.print(line1);
  tft.setCursor(15, 105);
  tft.print(line2);
}

static void onOtaProgress(int current, int total)
{
  if (total <= 0)
    return;

  int barX = 15;
  int barY = 180;
  int barW = 290;
  int barH = 20;
  int fillW = (current * barW) / total;

  tft.drawRect(barX, barY, barW, barH, ST77XX_WHITE);
  tft.fillRect(barX + 1, barY + 1, barW - 2, barH - 2, ST77XX_BLACK);
  tft.fillRect(barX + 1, barY + 1, fillW - 2 > 0 ? fillW - 2 : 0, barH - 2, ST77XX_GREEN);
}



static bool checkFirmwareAvailable()
{

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(7000);
  if (!http.begin(client, firmwareURL))
    return false;

  int code = http.sendRequest("HEAD");
  if (code <= 0 || code == HTTP_CODE_METHOD_NOT_ALLOWED)
  {
    http.end();
    if (!http.begin(client, firmwareURL))
      return false;
    code = http.GET();
  }

  http.end();
  return (code == HTTP_CODE_OK);
}

static void runOtaUpdate()
{
  otaInProgress = true;

  if (String(firmwareURL).indexOf("REPLACE_") >= 0)
  {
    drawStatusMessage("Set firmwareURL", "in update.cpp", ST77XX_YELLOW);
    delay(1800);
    drawUpdateScreen();
    otaInProgress = false;
    return;
  }

  drawStatusMessage("OTA Update", "Downloading...", ST77XX_WHITE);
  httpUpdate.onProgress(onOtaProgress);
  httpUpdate.rebootOnUpdate(false);

  WiFiClientSecure client;
  client.setInsecure();

  t_httpUpdate_return result = httpUpdate.update(client, firmwareURL);
  if (result == HTTP_UPDATE_OK)
  {
    drawStatusMessage("", "Rebooting...", ST77XX_WHITE);
    delay(1000);
    ESP.restart();
    return;
  }

  if (result == HTTP_UPDATE_NO_UPDATES)
  {
    drawStatusMessage("No Update", "Already latest", ST77XX_YELLOW);
    delay(1500);
    drawUpdateScreen();
    otaInProgress = false;
    return;
  }

  String err = httpUpdate.getLastErrorString();
  drawStatusMessage("Update Failed", err.substring(0, 20).c_str(), ST77XX_RED);
  delay(2000);
  drawUpdateScreen();
  otaInProgress = false;
}

void drawUpdateScreen()
{
  tft.fillScreen(ST77XX_BLACK);
  drawStatusMessage("", "Checking for update...", ST77XX_WHITE);
  firmwareAvailable = checkFirmwareAvailable();
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 5);
  tft.println("UPDATE");

  tft.setTextSize(2);
  tft.setCursor(10, 35);
  tft.print("Firmware: ");
  tft.print(FW_VERSION);

  tft.setCursor(10, 50);
  tft.print("Name: ");
  tft.print(firmwareName);

  if (String(firmwareURL).indexOf("REPLACE_") >= 0)
  {
    firmwareAvailable = false;
  }
  else
  {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 5);
    tft.println("UPDATE");

    tft.setTextSize(1);
    tft.setCursor(10, 35);
    tft.print("Firmware: ");
    tft.print(FW_VERSION);

    tft.setCursor(10, 50);
    tft.print("Name: ");
    tft.print(firmwareName);

    if (firmwareAvailable)
    {
      tft.drawRect(20, 120, 200, 45, ST77XX_CYAN);
      tft.setTextSize(2);
      tft.setCursor(50, 135);
      tft.setTextColor(ST77XX_CYAN);
      tft.print("UPDATE");
    }
    else
    {
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(10, 120);
      tft.print("No update available");
    }
  }
}

void handleUpdateInput()
{
  static bool lastBtnState = HIGH;
  bool btnState = digitalRead(ENC_BTN);

  if (!otaInProgress && firmwareAvailable && lastBtnState == HIGH && btnState == LOW)
  {
    runOtaUpdate();
  }
  lastBtnState = btnState;
}
