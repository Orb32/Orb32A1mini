#include "app_context.h"
#include <time.h>

Preferences prefs;
Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);

// ---------- ENCODER ----------
volatile int encoderValue = 0;
volatile int lastEncoded = 0;
unsigned long lastMove = 0;
unsigned long lastKeyMove = 0;

// ---------- WIFI ----------
String ssids[15];
int wifiCount = 0;
int menuIndex = 0;
String password = "";
int selectedWiFi = 0;

Screen screen = HOME;

// KEYBOARD
bool upperCase = false;
unsigned long koPressTime = 0;

const char keys[3][12] = {
    {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '@', '^'},
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '#', '<'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '_', '?', '>'},
};
int keyX = 0, keyY = 0;

const char *homeMenuConnected[4] = {
    "Clock",
    "Graph View",
    "Weather",
    "Settings"};

const char *homeMenuNoWiFi[3] = {
    "Connect WiFi",
    "Settings",
    "Reconnect WiFi"};

int homeIndex = 0;

const char *settingsMenu[5] = {
    "Update Firmware",
    "About",
    "WiFi Information",
    "Forget WiFi",
    "Shutdown"};

int settingsIndex = 0;
bool wifiInfoBtnArmed = false;

CoinItem coinList[MAX_COINS];
int coinCount = 0;

bool marketLoaded = false;

int coinIndex = 0;
int coinTopIndex = 0;
char selectedCoinID[32];
char selectedCoinSymbol[16];

// -------- COINGECKO --------
String coinSymbol = "";
String coinID = "";
String apiURL =
    "https://api.coingecko.com/api/v3/coins/" + coinID + "/market_chart?vs_currency=usd&days=1";

const int marketPerPage = 50;
int marketPage = 1;

// -------- STOCKS --------
const StockItem stockList[] = {
    {"Apple", "AAPL"},
    {"Microsoft", "MSFT"},
    {"NVIDIA", "NVDA"},
    {"Alphabet", "GOOGL"},
    {"Amazon", "AMZN"},
    {"Meta Platforms", "META"},
    {"Tesla", "TSLA"},
    {"Berkshire Hathaway", "BRK.B"},
    {"JPMorgan Chase", "JPM"},
    {"Visa", "V"},
    {"Mastercard", "MA"},
    {"Netflix", "NFLX"},
    {"Adobe", "ADBE"},
    {"Intel", "INTC"},
    {"AMD", "AMD"},
    {"Coca-Cola", "KO"},
    {"PepsiCo", "PEP"},
    {"Walmart", "WMT"},
    {"Costco", "COST"},
    {"Take-Two Interactive", "TTWO"},
    {"Exxon Mobil", "XOM"},
};

const int stockCount = sizeof(stockList) / sizeof(stockList[0]);
int stockIndex = 0;
int stockTopIndex = 0;
char selectedStockSymbol[16];
char selectedStockName[STOCK_NAME_LEN];
String stockApiKey = "5302cc8643314c5caaf61629cb7efea6";
int stockMarketState = -1;
unsigned long lastMarketStateFetch = 0;

// -------- FOREX --------
const ForexItem forexList[] = {
    {"EUR/USD"},
    {"GBP/USD"},
    {"USD/JPY"},
    {"USD/CHF"},
    {"AUD/USD"},
    {"USD/CAD"},
    {"NZD/USD"},
    {"EUR/GBP"},
    {"EUR/JPY"},
    {"GBP/JPY"},
    {"EUR/CHF"},
    {"GBP/CHF"},
    {"AUD/JPY"},
    {"EUR/AUD"},
    {"EUR/CAD"},
    {"EUR/NZD"},
    {"GBP/AUD"},
    {"GBP/CAD"},
    {"GBP/NZD"},
    {"CAD/JPY"},
};

const int forexCount = sizeof(forexList) / sizeof(forexList[0]);
int forexIndex = 0;
int forexTopIndex = 0;
char selectedForexSymbol[FOREX_SYMBOL_LEN];

// -------- GRAPH SOURCE --------
GraphSource graphSource = GRAPH_CRYPTO;
String graphName = "";
String graphSymbol = "";

// -------- GRAPH --------
float history[12];
bool firstLoad = true;

bool screenLocked = false;

// -------- WEATHER --------
WeatherInfo weatherInfo = {0, 0, 0, 0, "", false};
bool weatherLoaded = false;
bool weatherLocLoaded = false;
unsigned long lastWeatherFetch = 0;
float weatherLat = 0;
float weatherLon = 0;
String weatherCity = "";
String weatherRegion = "";
String weatherCountry = "";

// -------- TIME --------
static int timeOffsetSec = 0;
static bool timeConfigured = false;
static unsigned long lastTimeSync = 0;
static String lastHomeTimeStr = "";
static String lastWeatherTimeStr = "";
static String lastClockViewStr = "";

static String urlEncodeSymbol(const char *symbol)
{
  String encoded;
  for (const char *p = symbol; *p; ++p)
  {
    if (*p == '/')
      encoded += "%2F";
    else
      encoded += *p;
  }
  return encoded;
}

void IRAM_ATTR readEncoder()
{
  int MSB = digitalRead(ENC_A);
  int LSB = digitalRead(ENC_B);
  int encoded = (MSB << 2) | LSB;
  int sum = (lastEncoded << 2) | encoded;
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderValue++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderValue--;
  lastEncoded = encoded;
}

// Centered Text
void drawCenteredText(const char *text, int textSize)
{
  tft.setTextSize(textSize);

  int16_t x1, y1;
  uint16_t w, h;

  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int x = (tft.width() - w) / 2;
  int y = (tft.height() - h) / 2;

  tft.setCursor(x, y);
  tft.println(text);
}

// Detect KO button long press
bool isKOLongPress(unsigned long durationMs = 2000)
{
  static unsigned long pressStart = 0;
  static bool wasPressed = false;

  bool pressed = digitalRead(KO_BTN) == LOW;

  if (pressed && !wasPressed)
  {
    pressStart = millis();
    wasPressed = true;
  }

  if (!pressed && wasPressed)
  {
    wasPressed = false;
    pressStart = 0;
  }

  if (pressed && wasPressed && millis() - pressStart >= durationMs)
  {
    return true;
  }

  return false;
}

// Home menu item
void drawListItem(int y, const char *label, bool selected)
{
  int x = 20;
  int w = 280;
  int h = 37;

  uint16_t border = selected ? ST77XX_BLUE : ST77XX_WHITE;

  tft.drawRoundRect(x, y, w, h, 6, border);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(x + 10, y + 12);
  tft.print(label);
}

// Icons
// Home Icon
void drawHomeIcon(int x, int y, uint16_t color)
{
  tft.fillTriangle(x, y + 14, x + 20, y, x + 40, y + 14, color);
  tft.fillRect(x + 8, y + 14, 24, 22, color);
  tft.fillRect(x + 18, y + 24, 6, 12, ST77XX_BLACK);
}
// Wifi Icon
void drawWiFiIcon(int x, int y, int strength, uint16_t color)
{
  int barW = 5;
  int gap = 2;

  for (int i = 0; i < 4; i++)
  {
    int h = (i + 1) * 4;
    int bx = x + i * (barW + gap);
    int by = y + (24 - h);

    if (i < strength)
      tft.fillRect(bx, by, barW, h, color);
    else
      tft.drawRect(bx, by, barW, h, color);
  }
}

bool fetchIPTimeOffset(int &offsetSec)
{
  if (WiFi.status() != WL_CONNECTED)
    return false;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  if (!https.begin(client, "https://ipapi.co/json/"))
    return false;

  int httpCode = https.GET();
  if (httpCode != 200)
  {
    https.end();
    return false;
  }

  String payload = https.getString();
  https.end();

  JsonDocument doc;
  if (deserializeJson(doc, payload))
    return false;

  const char *utcOffset = doc["utc_offset"] | "";
  if (strlen(utcOffset) < 5)
    return false;

  int sign = (utcOffset[0] == '-') ? -1 : 1;
  int hours = (utcOffset[1] - '0') * 10 + (utcOffset[2] - '0');
  int mins = (utcOffset[3] - '0') * 10 + (utcOffset[4] - '0');

  offsetSec = sign * (hours * 3600 + mins * 60);
  return true;
}

bool ensureLocalTime()
{
  if (WiFi.status() != WL_CONNECTED)
    return false;

  const unsigned long resyncMs = 6UL * 60UL * 60UL * 1000UL;
  if (!timeConfigured || (millis() - lastTimeSync > resyncMs))
  {
    int offsetSec = 0;
    if (!fetchIPTimeOffset(offsetSec))
      return false;

    timeOffsetSec = offsetSec;
    configTime(timeOffsetSec, 0, "pool.ntp.org", "time.nist.gov");
    lastTimeSync = millis();
    timeConfigured = true;
  }

  return true;
}

String formatHomeTime()
{
  struct tm timeinfo;
  if (ensureLocalTime() && getLocalTime(&timeinfo, 1000))
  {
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    return String(buf);
  }

  return "";
}

void drawHomeTime(const String &timeStr)
{
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.fillRect(10, 10, 160, 16, ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.print("HOME  ");
  if (WiFi.status() == WL_CONNECTED)
  {
    if (timeStr.length() == 0)
    {
      tft.print("init...");
    }

    tft.print(timeStr);
  }
}

void drawWeatherTime(const String &timeStr)
{
  tft.setTextSize(4);
  tft.setTextColor(ST77XX_WHITE);
  tft.fillRect(150, 55, 120, 32, ST77XX_BLACK);
  tft.setCursor(150, 55);
  tft.print(timeStr);
}

void updateHomeTimeIfNeeded()
{
  if (screen != HOME && screen != WEATHER)
    return;

  String timeStr = formatHomeTime();

  if (timeStr.length() == 0)
    return;

  if (screen == HOME && timeStr != lastHomeTimeStr)
  {
    lastHomeTimeStr = timeStr;
    drawHomeTime(timeStr);
  }
  else if (screen == WEATHER && timeStr != lastWeatherTimeStr)
  {
    lastWeatherTimeStr = timeStr;
    drawWeatherTime(timeStr);
  }
}

int getHomeMenuCount()
{
  return (WiFi.status() == WL_CONNECTED)
             ? 4
         : hasSavedWiFi() ? 3
                          : 2;
}

const char *getHomeMenuItem(int index)
{
  if (WiFi.status() == WL_CONNECTED)
    return homeMenuConnected[index];
  return homeMenuNoWiFi[index];
}

// Get Clock info
bool getClockStrings(String &timeStr, String &dayStr, String &monthYearStr)
{
  struct tm timeinfo;
  if (ensureLocalTime() && getLocalTime(&timeinfo, 1000))
  {
    static const char *DAYS[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static const char *MONTHS[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"};

    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

    int year = timeinfo.tm_year + 1900 + 543;
    char monthYearBuf[32];
    snprintf(monthYearBuf, sizeof(monthYearBuf), "%d %s %d",
             timeinfo.tm_mday, MONTHS[timeinfo.tm_mon], year);

    timeStr = String(timeBuf);
    dayStr = String(DAYS[timeinfo.tm_wday]);
    monthYearStr = String(monthYearBuf);
    return true;
  }

  return false;
}

void updateClockViewIfNeeded()
{
  if (screen != CLOCK_VIEW)
    return;

  String timeStr = "";
  String dayStr = "";
  String monthYearStr = "";

  if (!getClockStrings(timeStr, dayStr, monthYearStr))
    return;

  String combined = timeStr + "|" + dayStr + "|" + monthYearStr;
  if (combined != lastClockViewStr)
  {
    lastClockViewStr = combined;
    drawClockTime(timeStr, dayStr, monthYearStr);
  }
}

// HOME Screen
void drawHome()
{
  tft.fillScreen(ST77XX_BLACK);
  drawHomeTime(lastHomeTimeStr);

  // WiFi bars
  if (WiFi.status() == WL_CONNECTED)
  {

    int rssi = WiFi.RSSI();
    int bars = 0;
    if (rssi > -50)
      bars = 4;
    else if (rssi > -60)
      bars = 3;
    else if (rssi > -70)
      bars = 2;
    else if (rssi > -80)
      bars = 1;

    tft.setCursor(210, 10);
    tft.print("2.4G ");
    drawWiFiIcon(270, 1, bars, ST77XX_WHITE);
  }
  else
  {
    drawWiFiIcon(270, 1, 0, ST77XX_WHITE);
  }

  int count = getHomeMenuCount();
  if (homeIndex >= count)
    homeIndex = 0;

  // List items
  for (int i = 0; i < count; i++)
  {
    drawListItem(50 + i * 45, getHomeMenuItem(i), homeIndex == i);
  }
}

// Welcome Screen
void drawWelcome()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  drawCenteredText("Orb32 A1 mini", 3);
}

// Nav back KO button
void goBack()
{
  switch (screen)
  {
  case PASSWORD:
    screen = WIFI_LIST;
    drawWiFiList();
    break;

  case WIFI_LIST:
  case GRAPH_MENU:
  case WEATHER:
  case CLOCK_VIEW:
  case SETTINGS:
    screen = HOME;
    drawHome();
    break;

  case SETTINGS_ABOUT:
  case SETTINGS_WIFI_INFO:
  case SETTINGS_UPDATE:
    screen = SETTINGS;
    drawSettings();
    break;

  case WIFI_FAIL:
    screen = WIFI_LIST;
    drawWiFiList();
    break;

  case LIVE_GRAPH_DETAIL:
    if (graphSource == GRAPH_STOCK)
    {
      screen = STOCK_VIEW;
      drawStockList();
    }
    else if (graphSource == GRAPH_FOREX)
    {
      screen = FOREX_VIEW;
      drawForexList();
    }
    else
    {
      screen = GRAPH_VIEW;
      drawCoinMarketList();
    }
    break;

  case STOCK_VIEW:
  case GRAPH_VIEW:
  case FOREX_VIEW:
    screen = GRAPH_MENU;
    drawGraphMenu();
    break;

  default:
    break;
  }
}

// SETUP
void setup()
{
  Serial.begin(115200);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);
  pinMode(KO_BTN, INPUT_PULLUP);
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  attachInterrupt(digitalPinToInterrupt(ENC_A), readEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), readEncoder, CHANGE);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(3);
  tft.invertDisplay(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);

  WiFi.mode(WIFI_STA);
  digitalWrite(TFT_BLK, HIGH);

  drawWelcome();
  delay(3000);

  if (hasSavedWiFi())
  {
    tft.fillScreen(ST77XX_BLACK);
    drawCenteredText("Connecting WiFi...", 2);
    bool wifiOK = autoConnectWiFi();
    showWiFiStatus(wifiOK);
    delay(2000);
    drawHome();
  }
  else
  {
    drawHome();
  }
}

// LOOP
void loop()
{

  if (screenLocked == true)
  {
    if (isKOLongPress())
    {
      digitalWrite(TFT_BLK, HIGH);
      encoderValue = 0;
      drawWelcome();
      delay(3000);
      drawHome();
      screenLocked = false;
    }
    return;
  }

  if (!digitalRead(KO_BTN))
  {
    goBack();
    delay(300);
    return;
  }

  static int lastEnc = 0;
  int val = encoderValue;

  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 1000)
  {
    updateHomeTimeIfNeeded();
    updateClockViewIfNeeded();
    lastUpdate = millis();
  }

  int diff = val - lastEnc;
  if (diff > 0)
    diff = 1;
  if (diff < 0)
    diff = -1;

  if (millis() - lastMove < 120)
    return;
  lastMove = millis();

  // HOME
  if (screen == HOME && val != lastEnc)
  {
    int count = getHomeMenuCount();
    homeIndex = (homeIndex + diff + count) % count;
    drawHome();
  }
  else if (screen == HOME && !digitalRead(ENC_BTN))
  {

    if (WiFi.status() == WL_CONNECTED)
    {
      switch (homeIndex)
      {
      case 0: // Clock
        screen = CLOCK_VIEW;
        lastClockViewStr = "";
        drawClockView();
        break;

      case 1: // Graph View
        screen = GRAPH_MENU;
        graphSource = GRAPH_CRYPTO;
        graphName = "";
        graphSymbol = "";
        delay(100);
        drawGraphMenu();
        break;

      case 2: // Weather
        screen = WEATHER;
        weatherLoaded = false;
        weatherLocLoaded = false;
        weatherInfo.valid = false;
        lastWeatherFetch = 0;
        drawWeather();
        break;

      case 3:
        settingsIndex = 0;
        screen = SETTINGS;
        delay(100);
        drawSettings();
        break;
      }
    }
    else
    {
      switch (homeIndex)
      {
      case 0: // Connect WiFi
        deleteSavedWiFi();
        scanWiFi();
        screen = WIFI_LIST;
        break;

      case 1: // Settings
        settingsIndex = 0;
        screen = SETTINGS;
        delay(100);
        drawSettings();
        break;
      case 2: // Reconnect WiFi
        if (hasSavedWiFi())
        {
          tft.fillScreen(ST77XX_BLACK);
          drawCenteredText("Reconnect WiFi...", 2);
          bool wifiOK = autoConnectWiFi();
          showWiFiStatus(wifiOK);
          delay(2000);
          screen = HOME;
          drawHome();
        }
        break;
      }
    }
  }

  // GRAPH MENU
  else if (screen == GRAPH_MENU && val != lastEnc)
  {
    if (diff > 0)
    {
      if (graphSource == GRAPH_CRYPTO)
        graphSource = GRAPH_STOCK;
      else if (graphSource == GRAPH_STOCK)
        graphSource = GRAPH_FOREX;
      else
        graphSource = GRAPH_CRYPTO;
    }
    else if (diff < 0)
    {
      if (graphSource == GRAPH_CRYPTO)
        graphSource = GRAPH_FOREX;
      else if (graphSource == GRAPH_FOREX)
        graphSource = GRAPH_STOCK;
      else
        graphSource = GRAPH_CRYPTO;
    }
    drawGraphMenu();
  }
  else if (screen == GRAPH_MENU && !digitalRead(ENC_BTN))
  {
    if (graphSource == GRAPH_CRYPTO)
    {
      screen = GRAPH_VIEW;
      marketLoaded = false;
      marketPage = 1;
      coinIndex = 0;
      coinTopIndex = 0;
      tft.fillScreen(ST77XX_BLACK);
      if (WiFi.status() == WL_CONNECTED)
      {
        drawCenteredText("Loading market...", 2);
      }
      else
      {
        drawCenteredText("WiFi not connected", 2);
        delay(2000);
        screen = HOME;
        drawHome();
      }
    }
    else if (graphSource == GRAPH_STOCK)
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        screen = STOCK_VIEW;
        stockIndex = 0;
        stockTopIndex = 0;
        drawStockList();
      }
      else
      {
        drawCenteredText("WiFi not connected", 2);
        delay(2000);
        screen = HOME;
        drawHome();
      }
    }
    else
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        screen = FOREX_VIEW;
        forexIndex = 0;
        forexTopIndex = 0;
        drawForexList();
      }
      else
      {
        drawCenteredText("WiFi not connected", 2);
        delay(2000);
        screen = HOME;
        drawHome();
      }
    }
    delay(200);
  }

  // WIFI LIST
  else if (screen == WIFI_LIST && val != lastEnc)
  {
    menuIndex = constrain(menuIndex + diff, 0, wifiCount - 1);
    drawWiFiList();
  }
  else if (screen == WIFI_LIST && !digitalRead(ENC_BTN))
  {
    selectedWiFi = menuIndex;
    password = "Kate0985656623";
    keyX = keyY = 0;
    screen = PASSWORD;
    drawKeyboard();
    delay(300);
  }

  // PASSWORD SCREEN
  else if (screen == PASSWORD)
  {

    // rotate = move cursor (auto row wrap)
    if (val != lastEnc && millis() - lastKeyMove > 01)
    {
      int dir = diff; // +1 or -1

      keyX += dir;

      // RIGHT EDGE → go to next row
      if (keyX > 11)
      {
        keyX = 0;
        keyY = (keyY + 1) % 3;
      }

      // LEFT EDGE → go to previous row
      else if (keyX < 0)
      {
        keyX = 11;
        keyY = (keyY + 2) % 3; // -1 mod 3
      }

      drawKeyboard();
      lastKeyMove = millis();
    }

    // ENC_BTN = select key
    if (!digitalRead(ENC_BTN))
    {
      char c = keys[keyY][keyX];

      // SHIFT KEY
      if (c == '^')
      {
        upperCase = !upperCase;
        drawKeyboard();
        delay(250);
      }

      // BACKSPACE
      else if (c == '<' && password.length())
      {
        password.remove(password.length() - 1);
        drawKeyboard();
        delay(150);
      }
      else if (c == '>' && password.length() > 0)
      {
        screen = CONNECTING;
        WiFi.begin(ssids[selectedWiFi].c_str(), password.c_str());

        tft.fillScreen(ST77XX_BLACK);
        drawCenteredText("Connecting...", 2);

        unsigned long start = millis();
        while (millis() - start < 8000)
        {
          if (WiFi.status() == WL_CONNECTED)
          {
            // Save WiFi credentials
            prefs.begin("wifi", false);
            prefs.putString("ssid", ssids[selectedWiFi]);
            prefs.putString("pass", password);
            prefs.end();

            screen = WIFI_CONNECTED;
            drawWiFiConnected();
            delay(2000);
            screen = HOME;
            drawHome();
            return;
          }
        }

        screen = WIFI_FAIL;
        drawWiFiFail();
        password = "";
        delay(2000);
        screen = PASSWORD;
        drawKeyboard();
      }
      // NORMAL CHAR
      else if (c != '<')
      {
        if (upperCase && c >= 'a' && c <= 'z')
          c -= 32;
        password += c;
        drawKeyboard();
        delay(150);
      }
    }
  }

  // WEATHER SCREEN
  if (screen == WEATHER)
  {
    static unsigned long lastAttempt = 0;
    const unsigned long refreshMs = 10UL * 60UL * 1000UL;

    if (!weatherLoaded || (millis() - lastWeatherFetch > refreshMs))
    {
      if (millis() - lastAttempt < 2000)
        return;

      lastAttempt = millis();

      if (!weatherLocLoaded)
      {
        if (!fetchIPLocation(weatherLat, weatherLon, weatherCity, weatherRegion, weatherCountry))
        {
          drawWeatherError("Location error");
          delay(2000);
          drawHome();
          screen = HOME;
          return;
        }
        weatherLocLoaded = true;
      }

      if (!fetchCurrentWeather(weatherLat, weatherLon, weatherInfo))
      {
        drawWeatherError("Weather error");
        delay(2000);
        drawHome();
        screen = HOME;
        return;
      }

      weatherLoaded = true;
      lastWeatherFetch = millis();
      drawWeather();
    }
    return;
  }

  // Coin Market Screen
  if (screen == GRAPH_VIEW)
  {
    const int extraTop = (marketPage > 1) ? 1 : 0;
    const int totalRows = coinCount + extraTop + 1; // + Next Page

    // Load market ONCE
    if (!marketLoaded)
    {
      if (fetchCoinMarketList())
      {
        drawCoinMarketList();
        marketLoaded = true;
      }
      return;
    }
    // Scroll coins
    if (val != lastEnc)
    {
      coinIndex = constrain(coinIndex + diff, 0, max(0, totalRows - 1));
      drawCoinMarketList();
    }
    // SELECT coin
    if (!digitalRead(ENC_BTN))
    {
      if (extraTop && coinIndex == 0)
      {
        marketPage = max(1, marketPage - 1);
        marketLoaded = false;
        coinIndex = 0;
        coinTopIndex = 0;
        tft.fillScreen(ST77XX_BLACK);
        drawCenteredText("Loading market...", 2);
        delay(300);
        return;
      }
      if (coinIndex == totalRows - 1)
      {
        marketPage++;
        marketLoaded = false;
        coinIndex = 0;
        coinTopIndex = 0;
        tft.fillScreen(ST77XX_BLACK);
        drawCenteredText("Loading market...", 2);
        delay(300);
        return;
      }

      int coinListIndex = coinIndex - extraTop;
      strcpy(selectedCoinID, coinList[coinListIndex].id);
      strcpy(selectedCoinSymbol, coinList[coinListIndex].symbol);

      coinSymbol = selectedCoinSymbol;
      coinID = selectedCoinID;
      graphName = coinList[coinListIndex].name;
      graphSymbol = coinSymbol;
      graphSource = GRAPH_CRYPTO;
      apiURL =
          "https://api.coingecko.com/api/v3/coins/" + coinID +
          "/market_chart?vs_currency=usd&days=1";

      firstLoad = true;
      screen = LIVE_GRAPH_DETAIL;

      drawFrame();
      tft.setCursor(15, 70);
      tft.setTextColor(ST77XX_WHITE);
      tft.print("Loading...");
      delay(300);
      return;
    }
  }

  // Stock List Screen
  if (screen == STOCK_VIEW)
  {
    const int totalRows = stockCount;

    if (val != lastEnc)
    {
      stockIndex = constrain(stockIndex + diff, 0, max(0, totalRows - 1));
      drawStockList();
    }
    if (!digitalRead(ENC_BTN))
    {
      strcpy(selectedStockSymbol, stockList[stockIndex].symbol);
      strncpy(selectedStockName, stockList[stockIndex].name, STOCK_NAME_LEN - 1);
      selectedStockName[STOCK_NAME_LEN - 1] = '\0';

      graphName = selectedStockName;
      graphSymbol = selectedStockSymbol;
      graphSource = GRAPH_STOCK;

      apiURL = "https://api.twelvedata.com/time_series?symbol=" + String(selectedStockSymbol) +
               "&interval=2h&outputsize=12&order=asc&apikey=" + stockApiKey;

      firstLoad = true;
      screen = LIVE_GRAPH_DETAIL;

      drawFrame();
      tft.setCursor(15, 70);
      tft.setTextColor(ST77XX_WHITE);
      tft.print("Loading...");
      delay(300);
      return;
    }
  }

  // Forex List Screen
  if (screen == FOREX_VIEW)
  {
    const int totalRows = forexCount;

    if (val != lastEnc)
    {
      forexIndex = constrain(forexIndex + diff, 0, max(0, totalRows - 1));
      drawForexList();
    }
    if (!digitalRead(ENC_BTN))
    {
      strncpy(selectedForexSymbol, forexList[forexIndex].symbol, FOREX_SYMBOL_LEN - 1);
      selectedForexSymbol[FOREX_SYMBOL_LEN - 1] = '\0';

      graphName = selectedForexSymbol;
      graphSymbol = selectedForexSymbol;
      graphSource = GRAPH_FOREX;

      String encodedSymbol = urlEncodeSymbol(selectedForexSymbol);
      apiURL = "https://api.twelvedata.com/time_series?symbol=" + encodedSymbol +
               "&interval=2h&outputsize=12&order=asc&apikey=" + stockApiKey;

      firstLoad = true;
      screen = LIVE_GRAPH_DETAIL;

      drawFrame();
      tft.setCursor(15, 70);
      tft.setTextColor(ST77XX_WHITE);
      tft.print("Loading...");
      delay(300);
      return;
    }
  }

  if (screen == LIVE_GRAPH_DETAIL)
  {
    static unsigned long lastUpdate = 0;
    static unsigned long lastTimeDraw = 0;

    const unsigned long timeRefreshMs = 60UL * 1000UL;
    if (millis() - lastTimeDraw > timeRefreshMs)
    {
      drawGraphTime();
      lastTimeDraw = millis();
    }

    const unsigned long stockRefreshMs = 2UL * 60UL * 60UL * 1000UL;

    if ((graphSource == GRAPH_STOCK && (millis() - lastUpdate > stockRefreshMs || firstLoad)) ||
        (graphSource == GRAPH_CRYPTO && (millis() - lastUpdate > 15000 || firstLoad)) ||
        (graphSource == GRAPH_FOREX && (millis() - lastUpdate > stockRefreshMs || firstLoad)))
    {
      lastUpdate = millis();

      float price = 0, change = 0;

      if (fetchLast12(price, change))
      {
        if (firstLoad)
          firstLoad = false;

        drawFrame();
        showData(price, change);
        drawGraph();
      }
    }
    return;
  }

  // SETTINGS SCREEN
  else if (screen == SETTINGS && val != lastEnc)
  {
    settingsIndex = (settingsIndex + diff + 5) % 5;
    drawSettings();
  }
  else if (screen == SETTINGS && !digitalRead(ENC_BTN))
  {
    switch (settingsIndex)
    {
    case 0: // Update Firmware
      if (WiFi.status() == WL_CONNECTED)
  {
      screen = SETTINGS_UPDATE;
      drawUpdateScreen();
  }else{
      tft.fillScreen(ST77XX_BLACK);
      drawCenteredText("WiFi not connected", 2);
      delay(2000);
      screen = SETTINGS;
      drawSettings();
  }
      break;

    case 1: // About
      screen = SETTINGS_ABOUT;
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextSize(2);
      tft.setCursor(10, 5);
      tft.println("Orb32 A1 mini v1.0");
      tft.println("\n By P&P iot solutions");
      break;

    case 2: // WiFi Info
      screen = SETTINGS_WIFI_INFO;
      wifiInfoBtnArmed = false;
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextSize(2);
      tft.setCursor(10, 5);

      if (WiFi.status() == WL_CONNECTED)
      {
        tft.println("WiFi SSID: " + WiFi.SSID());
        tft.println("\n IP: " + WiFi.localIP().toString());

        tft.setTextColor(ST77XX_BLUE);
        tft.println("\n\n Disconnect WiFi");
      }
      else
      {
        drawCenteredText("WiFi Not Connected", 2);
      }
      break;
    case 3: // Forget WiFi
    {
      tft.fillScreen(ST77XX_BLACK);
      drawCenteredText("Deleting WiFi credentials...", 1);

      deleteSavedWiFi();
      delay(1000);
      screen = HOME;
      drawHome();
    }
    break;

    case 4: // Shutdown
    {
      tft.fillScreen(ST77XX_BLACK);
      drawCenteredText("Shutting down...", 2);
      delay(1000);
      digitalWrite(TFT_BLK, LOW);
      screenLocked = true;
      screen = HOME;
      drawHome();
    }
    break;
    }
  }
  else if (screen == SETTINGS_UPDATE)
  {
    handleUpdateInput();
  }
  else if (screen == SETTINGS_WIFI_INFO)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      if (digitalRead(ENC_BTN))
        wifiInfoBtnArmed = true;

      if (wifiInfoBtnArmed && !digitalRead(ENC_BTN))
      {
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
        drawCenteredText("Disconnecting...", 2);
        WiFi.disconnect();
        delay(1000);
        screen = HOME;
        drawHome();
      }
    }
  }

  lastEnc = val;
}
