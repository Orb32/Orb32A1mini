#include "app_context.h"

static void drawGraphMenuItem(int y, const char *label, bool selected)
{
  int x = 20;
  int w = 280;
  int h = 40;
  uint16_t border = selected ? ST77XX_BLUE : ST77XX_WHITE;
  tft.drawRoundRect(x, y, w, h, 6, border);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(x + 10, y + 12);
  tft.print(label);
}

static void drawGraphMenuItemStatus(int y, const char *label, const char *status, bool selected)
{
  int x = 20;
  int w = 280;
  int h = 40;
  uint16_t border = selected ? ST77XX_BLUE : ST77XX_WHITE;
  tft.drawRoundRect(x, y, w, h, 6, border);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(x + 10, y + 12);
  tft.print(label);

  if (status && status[0])
  {
    int16_t x1, y1;
    uint16_t tw, th;
    tft.getTextBounds(label, 0, 0, &x1, &y1, &tw, &th);
    tft.setTextColor(DARKGREY);
    tft.setCursor(x + 10 + tw + 6, y + 12);
    tft.print(status);
  }
}

static bool fetchMarketState(bool &isOpen)
{
  if (WiFi.status() != WL_CONNECTED)
    return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.setTimeout(4000);

  String url = "https://api.twelvedata.com/market_state?apikey=" + stockApiKey;
  if (!https.begin(client, url))
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
  DeserializationError err = deserializeJson(doc, payload);
  if (err)
    return false;

  JsonArray arr;
  if (doc.is<JsonArray>())
    arr = doc.as<JsonArray>();
  else if (doc.is<JsonObject>() && doc["data"].is<JsonArray>())
    arr = doc["data"].as<JsonArray>();
  else
    return false;

  bool fallbackFound = false;
  bool fallbackOpen = false;

  for (JsonObject item : arr)
  {
    const char *code = item["code"] | "";
    const char *name = item["name"] | "";
    const char *country = item["country"] | "";
    bool open = item["is_market_open"] | false;

    if (strcmp(code, "XNYS") == 0 || strcmp(name, "NYSE") == 0)
    {
      isOpen = open;
      return true;
    }
    if (strcmp(code, "XNAS") == 0 || strcmp(name, "NASDAQ") == 0)
    {
      isOpen = open;
      return true;
    }

    if (!fallbackFound && strcmp(country, "United States") == 0)
    {
      fallbackFound = true;
      fallbackOpen = open;
    }
  }

  if (fallbackFound)
  {
    isOpen = fallbackOpen;
    return true;
  }

  return false;
}

String buildMarketURL()
{
  return "https://api.coingecko.com/api/v3/coins/markets"
         "?vs_currency=usd"
         "&order=market_cap_desc"
         "&per_page=" + String(marketPerPage) +
         "&page=" + String(marketPage) +
         "&sparkline=false";
}

// Fetch Coin Market from Coingecko API
bool fetchCoinMarketList()
{
  if (WiFi.status() != WL_CONNECTED)
    return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  String url = buildMarketURL();
  if (!https.begin(client, url))
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

  DeserializationError err = deserializeJson(doc, payload);
  if (err)
    return false;

  coinCount = min((int)doc.size(), MAX_COINS);

  for (int i = 0; i < coinCount; i++)
  {
    const char *name = doc[i]["name"];
    const char *id = doc[i]["id"];
    const char *sym = doc[i]["symbol"];
    float price = doc[i]["current_price"];

    strncpy(coinList[i].name, name, COIN_NAME_LEN - 1);
    coinList[i].name[COIN_NAME_LEN - 1] = '\0';

    // store id & symbol in hidden buffers using name slots
    if (id && sym)
    {
      strncpy(coinList[i].id, id, 31);
      strncpy(coinList[i].symbol, sym, 15);
    }
    coinList[i].name[COIN_NAME_LEN - 1] = '\0';
    coinList[i].price = price;
  }

  return true;
}

// Graph Menu Screen
void drawGraphMenu()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 8);
  tft.print("Graph View");

  if (stockMarketState == -1)
  {
    drawCenteredText("Loading...", 2);
  }

  const unsigned long refreshMs = 5UL * 60UL * 1000UL;
  if (stockMarketState == -1 || millis() - lastMarketStateFetch > refreshMs)
  {
    bool isOpen = false;
    if (fetchMarketState(isOpen))
    {
      stockMarketState = isOpen ? 1 : 0;
      lastMarketStateFetch = millis();
      drawGraphMenu();
    }
  }

  drawGraphMenuItem(55, "Crypto", graphSource == GRAPH_CRYPTO);
  const char *status = "";
  if (stockMarketState == 1)
    status = "(Open)";
  else if (stockMarketState == 0)
    status = "(Close)";
  drawGraphMenuItemStatus(105, "Stock", status, graphSource == GRAPH_STOCK);
  drawGraphMenuItem(155, "Forex", graphSource == GRAPH_FOREX);
}

// Coin Market List Screen
void drawCoinMarketList()
{
  const int headerH = 30;
  const int visibleCount = 8;
  const int startY = headerH + 10;
  const int rowH = 25;
  const int extraTop = (marketPage > 1) ? 1 : 0;
  const int extraBottom = 1; // Next Page
  const int totalRows = coinCount + extraTop + extraBottom;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 6);
  tft.print("Market");

  // --- FIXED SCROLL WINDOW LOGIC ---
  if (coinIndex < coinTopIndex)
    coinTopIndex = coinIndex;
  else if (coinIndex >= coinTopIndex + visibleCount)
    coinTopIndex = coinIndex - visibleCount + 1;

  coinTopIndex = constrain(coinTopIndex, 0, max(0, totalRows - visibleCount));

  // --- DRAW LIST ---
  for (int i = 0; i < visibleCount; i++)
  {
    int listIndex = coinTopIndex + i;
    if (listIndex >= totalRows)
      break;

    int y = startY + i * rowH;

    // Hover color
    tft.setTextColor(listIndex == coinIndex ? ST77XX_BLUE : ST77XX_WHITE);

    // Prev/Next row
    if (extraTop && listIndex == 0)
    {
      tft.setCursor(10, y);
      tft.print("< Previous Page");
      continue;
    }
    if (listIndex == totalRows - 1)
    {
      tft.setCursor(10, y);
      tft.print("Next Page >");
      continue;
    }

    int index = listIndex - extraTop;

    // Coin name
    char nameBuf[16]; // 12 chars + "..." + null

    int maxChars = 12;
    int nameLen = strlen(coinList[index].name);

    if (nameLen > maxChars)
    {
      strncpy(nameBuf, coinList[index].name, maxChars);
      nameBuf[maxChars] = '\0';
      strcat(nameBuf, "...");
    }
    else
    {
      strncpy(nameBuf, coinList[index].name, sizeof(nameBuf) - 1);
      nameBuf[sizeof(nameBuf) - 1] = '\0';
    }
    tft.setCursor(10, y);
    tft.print(nameBuf);

    // Price (right aligned)
    tft.setTextColor(ST77XX_WHITE);
    char priceBuf[16];
    snprintf(priceBuf, sizeof(priceBuf), "$%.2f", coinList[index].price);

    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(priceBuf, 0, y, &x1, &y1, &w, &h);
    tft.setCursor(tft.width() - w - 8, y);
    tft.print(priceBuf);
  }
}

// Stock List Screen
void drawStockList()
{
  const int headerH = 30;
  const int visibleCount = 8;
  const int startY = headerH + 10;
  const int rowH = 25;
  const int totalRows = stockCount;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 6);
  tft.print("Stocks");

  if (stockIndex < stockTopIndex)
    stockTopIndex = stockIndex;
  else if (stockIndex >= stockTopIndex + visibleCount)
    stockTopIndex = stockIndex - visibleCount + 1;

  stockTopIndex = constrain(stockTopIndex, 0, max(0, totalRows - visibleCount));

  for (int i = 0; i < visibleCount; i++)
  {
    int listIndex = stockTopIndex + i;
    if (listIndex >= totalRows)
      break;

    int y = startY + i * rowH;

    tft.setTextColor(listIndex == stockIndex ? ST77XX_BLUE : ST77XX_WHITE);

    char nameBuf[16];
    int maxChars = 12;
    int nameLen = strlen(stockList[listIndex].name);

    if (nameLen > maxChars)
    {
      strncpy(nameBuf, stockList[listIndex].name, maxChars);
      nameBuf[maxChars] = '\0';
      strcat(nameBuf, "...");
    }
    else
    {
      strncpy(nameBuf, stockList[listIndex].name, sizeof(nameBuf) - 1);
      nameBuf[sizeof(nameBuf) - 1] = '\0';
    }
    tft.setCursor(10, y);
    tft.print(nameBuf);

    tft.setTextColor(ST77XX_WHITE);
    char symBuf[12];
    strncpy(symBuf, stockList[listIndex].symbol, sizeof(symBuf) - 1);
    symBuf[sizeof(symBuf) - 1] = '\0';

    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(symBuf, 0, y, &x1, &y1, &w, &h);
    tft.setCursor(tft.width() - w - 8, y);
    tft.print(symBuf);
  }
}

// Forex List Screen
void drawForexList()
{
  const int headerH = 30;
  const int visibleCount = 8;
  const int startY = headerH + 10;
  const int rowH = 25;
  const int totalRows = forexCount;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 6);
  tft.print("Forex");

  if (totalRows == 0)
  {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 60);
    tft.print("No data");
    return;
  }

  if (forexIndex < forexTopIndex)
    forexTopIndex = forexIndex;
  else if (forexIndex >= forexTopIndex + visibleCount)
    forexTopIndex = forexIndex - visibleCount + 1;

  forexTopIndex = constrain(forexTopIndex, 0, max(0, totalRows - visibleCount));

  for (int i = 0; i < visibleCount; i++)
  {
    int listIndex = forexTopIndex + i;
    if (listIndex >= totalRows)
      break;

    int y = startY + i * rowH;

    tft.setTextColor(listIndex == forexIndex ? ST77XX_BLUE : ST77XX_WHITE);

    tft.setCursor(10, y);
    tft.print(forexList[listIndex].symbol);
  }
}

// Fetch last crypto data points from API
static bool fetchLastCrypto(float &price, float &change)
{
  HTTPClient http;
  http.begin(apiURL);
  http.addHeader("User-Agent", "ESP32");
  int code = http.GET();

  if (code != 200)
  {
    Serial.println("HTTP ERROR");
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, payload))
    return false;

  JsonArray prices = doc["prices"];
  int total = prices.size();

  const int count = 12;
  if (total < count)
    return false;

  for (int i = 0; i < count; i++)
    history[i] = prices[total - count + i][1];

  float first = history[0];
  float last = history[count - 1];

  change = ((last - first) / first) * 100.0;
  price = last;

  return true;
}

// Fetch last stock data points from API
static bool fetchLastStock(float &price, float &change)
{
  HTTPClient http;
  http.begin(apiURL);
  http.addHeader("User-Agent", "ESP32");
  int code = http.GET();

  if (code != 200)
  {
    Serial.println("HTTP ERROR");
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, payload))
    return false;

  if (doc["status"] && strcmp(doc["status"], "error") == 0)
    return false;

  JsonArray values = doc["values"];
  int total = values.size();
  const int count = 12;
  if (total < count)
    return false;

  for (int i = 0; i < count; i++)
  {
    const char *closeStr = values[i]["close"] | "0";
    history[i] = atof(closeStr);
  }

  float first = history[0];
  float last = history[count - 1];

  change = ((last - first) / first) * 100.0;
  price = last;

  return true;
}

// Fetch last forex data points from API
static bool fetchLastForex(float &price, float &change)
{
  HTTPClient http;
  http.begin(apiURL);
  http.addHeader("User-Agent", "ESP32");
  int code = http.GET();

  if (code != 200)
  {
    Serial.println("HTTP ERROR");
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, payload))
    return false;

  if (doc["status"] && strcmp(doc["status"], "error") == 0)
    return false;

  JsonArray values = doc["values"];
  int total = values.size();
  const int count = 12;
  if (total < count)
    return false;

  for (int i = 0; i < count; i++)
  {
    const char *closeStr = values[i]["close"] | "0";
    history[i] = atof(closeStr);
  }

  float first = history[0];
  float last = history[count - 1];

  change = ((last - first) / first) * 100.0;
  price = last;

  return true;
}

// Fetch last data points from API (crypto or stock)
bool fetchLast12(float &price, float &change)
{
  if (graphSource == GRAPH_STOCK)
    return fetchLastStock(price, change);
  if (graphSource == GRAPH_FOREX)
    return fetchLastForex(price, change);

  return fetchLastCrypto(price, change);
}

void drawFrame()
{
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 8);
  if (graphSource == GRAPH_CRYPTO)
    tft.print("Live Graph (1m)");
  else
    tft.print("Live Graph (2h)");

  String timeStr = formatHomeTime();
  if (timeStr.length())
  {
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(tft.width() - w - 10, 8);
    tft.print(timeStr);
  }

  tft.drawRect(5, 35, 140, 200, ST77XX_WHITE);
  tft.drawRect(155, 35, 160, 200, ST77XX_WHITE);

  tft.setCursor(160, 40);
  tft.print("MAX");

  tft.setCursor(160, 215);
  tft.print("MIN");
}

void drawGraphTime()
{
  String timeStr = formatHomeTime();
  if (!timeStr.length())
    return;

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  // Clear the time area (top-right)
  tft.fillRect(tft.width() - 70, 6, 60, 20, ST77XX_BLACK);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(tft.width() - w - 10, 8);
  tft.print(timeStr);
}

void drawGraph()
{
  const int count = 12;
  int gx = 155, gy = 35, gw = 160, gh = 200;

  int px = gx + 10;
  int py = gy + 30;
  int pw = gw - 20;
  int ph = gh - 70;

  tft.fillRect(px - 4, py - 4, pw + 8, ph + 8, ST77XX_BLACK);

  float minP = history[0], maxP = history[0];
  for (int i = 0; i < count; i++)
  {
    minP = min(minP, history[i]);
    maxP = max(maxP, history[i]);
  }

  float range = maxP - minP;
  if (range <= 0)
    range = 0.000001;

  for (int i = 1; i < count; i++)
  {
    int x1 = px + (i - 1) * (pw / (count - 1));
    int x2 = px + i * (pw / (count - 1));

    float scale = ph / range;

    int y1 = py + ph - (history[i - 1] - minP) * scale;
    int y2 = py + ph - (history[i] - minP) * scale;

    tft.drawLine(x1, y1, x2, y2, ST77XX_RED);
    tft.fillCircle(x2, y2, 3, ST77XX_RED);
  }

  tft.drawRect(gx, gy, gw, gh, ST77XX_WHITE);
}

void showData(float price, float change)
{
  String displayCoin = graphName;
  tft.fillRect(10, 60, 130, 170, ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  displayCoin.toUpperCase();
  char nameBuf[16]; // 12 chars + "..." + null

  int maxChars = 8;
  int nameLen = strlen(displayCoin.c_str());

  if (nameLen > maxChars)
  {
    strncpy(nameBuf, displayCoin.c_str(), maxChars);
    nameBuf[maxChars] = '\0';
    strcat(nameBuf, "...");
  }
  else
  {
    strncpy(nameBuf, displayCoin.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
  }
  tft.setCursor(15, 55);
  tft.print(nameBuf);

  tft.setCursor(15, 80);
  if (graphSymbol.length())
  {
    graphSymbol.toUpperCase();
    tft.print("(" + graphSymbol + ")");
  }

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 110);
  tft.print("PRICE:");

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(15, 135);
  tft.print(price, 2);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 170);
  tft.print("CHANGE:");

  tft.setTextColor(change < 0 ? ST77XX_RED : ST77XX_GREEN);
  tft.setCursor(15, 195);
  tft.print(change, 2);
  tft.print(" %");
}
