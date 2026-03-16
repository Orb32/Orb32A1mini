#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define DARKGREY 0x7BEF // 16-bit gray color

// TFT
#define TFT_SCLK 4
#define TFT_MOSI 6
#define TFT_RST 7
#define TFT_DC 8
#define TFT_CS 9
#define TFT_BLK 10

// INPUT
#define ENC_A 2
#define ENC_B 3
#define ENC_BTN 5
#define KO_BTN 1

extern Preferences prefs;
extern Adafruit_ST7789 tft;

// ---------- ENCODER ----------
extern volatile int encoderValue;
extern volatile int lastEncoded;
extern unsigned long lastMove;
extern unsigned long lastKeyMove;

// ---------- WIFI ----------
extern String ssids[15];
extern int wifiCount;
extern int menuIndex;
extern String password;
extern int selectedWiFi;

enum Screen
{
  HOME,
  WIFI_LIST,
  PASSWORD,
  CONNECTING,
  WIFI_FAIL,
  GRAPH_MENU,
  GRAPH_VIEW,
  STOCK_VIEW,
  FOREX_VIEW,
  LIVE_GRAPH_DETAIL,
  WEATHER,
  SETTINGS,
  SETTINGS_UPDATE,
  SETTINGS_ABOUT,
  SETTINGS_WIFI_INFO,
  WIFI_CONNECTED,
  CLOCK_VIEW
};

extern Screen screen;

// KEYBOARD
extern bool upperCase;
extern unsigned long koPressTime;
extern const char keys[3][12];
extern int keyX;
extern int keyY;

extern int homeIndex;

extern const char *settingsMenu[5];
extern int settingsIndex;

#define MAX_COINS 50
#define COIN_NAME_LEN 24
#define STOCK_NAME_LEN 24
#define FOREX_SYMBOL_LEN 16

struct CoinItem
{
  char name[COIN_NAME_LEN];
  char id[32];
  char symbol[16];
  float price;
};

extern CoinItem coinList[MAX_COINS];
extern int coinCount;
extern bool marketLoaded;

extern int coinIndex;
extern int coinTopIndex;
extern char selectedCoinID[32];
extern char selectedCoinSymbol[16];

extern String coinSymbol;
extern String coinID;
extern String apiURL;

extern const int marketPerPage;
extern int marketPage;

// -------- STOCKS --------
struct StockItem
{
  char name[STOCK_NAME_LEN];
  char symbol[16];
};

extern const StockItem stockList[];
extern const int stockCount;
extern int stockIndex;
extern int stockTopIndex;
extern char selectedStockSymbol[16];
extern char selectedStockName[STOCK_NAME_LEN];
extern String stockApiKey;
extern int stockMarketState; // -1 unknown, 0 closed, 1 open
extern unsigned long lastMarketStateFetch;

// -------- FOREX --------
struct ForexItem
{
  const char *symbol;
};

extern const ForexItem forexList[];
extern const int forexCount;
extern int forexIndex;
extern int forexTopIndex;
extern char selectedForexSymbol[FOREX_SYMBOL_LEN];

// -------- GRAPH SOURCE --------
enum GraphSource
{
  GRAPH_CRYPTO,
  GRAPH_STOCK,
  GRAPH_FOREX
};

extern GraphSource graphSource;
extern String graphName;
extern String graphSymbol;

// -------- GRAPH --------
extern float history[12];
extern bool firstLoad;

extern bool screenLocked;

// -------- WEATHER --------
struct WeatherInfo
{
  float tempC;
  int humidity;
  float wind;
  int code;
  String time;
  bool valid;
};

extern WeatherInfo weatherInfo;
extern bool weatherLoaded;
extern bool weatherLocLoaded;
extern unsigned long lastWeatherFetch;
extern float weatherLat;
extern float weatherLon;
extern String weatherCity;
extern String weatherRegion;
extern String weatherCountry;

// Shared helpers
void drawCenteredText(const char *text, int textSize);
void drawWeather();
void drawHome();
String formatHomeTime();

// WiFi UI + logic
void deleteSavedWiFi();
void showWiFiStatus(bool connected);
bool hasSavedWiFi();
bool autoConnectWiFi();
void drawWiFiList();
void drawKeyboard();
void drawWiFiFail();
void drawWiFiConnected();
void scanWiFi();

// Graph view
String buildMarketURL();
bool fetchCoinMarketList();
void drawCoinMarketList();
void drawGraphMenu();
void drawStockList();
void drawForexList();
bool fetchLast12(float &price, float &change);
void drawFrame();
void drawGraphTime();
void drawGraph();
void showData(float price, float change);

// Weather
const char *weatherCodeToText(int code);
bool fetchIPLocation(float &lat, float &lon, String &city, String &region, String &country);
bool fetchCurrentWeather(float lat, float lon, WeatherInfo &info);
void drawWeatherError(const char *msg);
void drawWeatherTime(const String &timeStr);
bool getClockStrings(String &timeStr, String &dayStr, String &monthYearStr);
void drawClockView();
void drawClockTime(const String &timeStr, const String &dayStr, const String &monthYearStr);

// Settings
void drawSettings();

// Update
void drawUpdateScreen();
void handleUpdateInput();
