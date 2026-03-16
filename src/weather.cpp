#include "app_context.h"

// Map Open-Meteo weather codes to short text
const char *weatherCodeToText(int code)
{
  switch (code)
  {
  case 0:
    return "Clear";
  case 1:
  case 2:
    return "Partly Cloudy";
  case 3:
    return "Overcast";
  case 45:
  case 48:
    return "Fog";
  case 51:
  case 53:
  case 55:
    return "Drizzle";
  case 56:
  case 57:
    return "Freezing Drizzle";
  case 61:
  case 63:
  case 65:
    return "Rain";
  case 66:
  case 67:
    return "Freezing Rain";
  case 71:
  case 73:
  case 75:
    return "Snow";
  case 77:
    return "Snow Grains";
  case 80:
  case 81:
  case 82:
    return "Rain Showers";
  case 85:
  case 86:
    return "Snow Showers";
  case 95:
    return "Thunderstorm";
  case 96:
  case 99:
    return "Storm + Hail";
  default:
    return "Unknown";
  }
}

bool fetchIPLocation(float &lat, float &lon, String &city, String &region, String &country)
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

  lat = doc["latitude"] | 0.0;
  lon = doc["longitude"] | 0.0;
  const char *jsonCity = doc["city"] | "";
  const char *jsonRegion = doc["region"] | "";
  const char *jsonCountry = doc["country_code"] | "";

  weatherCity = jsonCity;
  weatherRegion = jsonRegion;
  weatherCountry = jsonCountry;

  return (lat != 0.0 || lon != 0.0);
}

bool fetchCurrentWeather(float lat, float lon, WeatherInfo &info)
{
  if (WiFi.status() != WL_CONNECTED)
    return false;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
               "&longitude=" + String(lon, 4) +
               "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code" +
               "&timezone=auto";

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
  if (deserializeJson(doc, payload))
    return false;

  JsonObject current = doc["current"];
  if (current.isNull())
    return false;

  info.tempC = current["temperature_2m"] | 0.0;
  info.humidity = current["relative_humidity_2m"] | 0;
  info.wind = current["wind_speed_10m"] | 0.0;
  info.code = current["weather_code"] | 0;
  info.time = current["time"] | "";
  info.valid = true;

  return true;
}

void drawWeatherError(const char *msg)
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 5);
  tft.println("Live Weather");
  tft.setTextColor(ST77XX_RED);
  tft.setCursor(10, 60);
  tft.println(msg);
}

// Weather Screen
void drawWeather()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 5);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Live Weather");

 
    if (weatherLoaded == false)
    {
      tft.setCursor(10, 40);
      drawCenteredText("Loading...", 2);
      return;
    }
  

  // Location line
  tft.setTextSize(1);
  tft.setTextColor(DARKGREY);
  tft.setCursor(10, 30);
  tft.print(weatherCity);
  if (weatherRegion.length())
  {
    tft.print(", ");
    tft.print(weatherRegion);
  }
  if (weatherCountry.length())
  {
    tft.print(" ");
    tft.print(weatherCountry);
  }

  // Temp
  tft.setTextSize(4);
  tft.setTextColor(ST77XX_BLUE);
  tft.setCursor(10, 55);
  tft.print(weatherInfo.tempC, 1);
  tft.print("C");

  // Time
  String timeStr = formatHomeTime();
  drawWeatherTime(timeStr);
  

  // Description
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLUE);
  tft.setCursor(10, 110);
  tft.print("Now: ");
  tft.print(weatherCodeToText(weatherInfo.code));

  // Details
  tft.setTextSize(2);
  tft.setCursor(10, 145);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Humidity: ");
  tft.setTextColor(weatherInfo.humidity > 65 ? ST77XX_RED : ST77XX_WHITE);
  tft.print(weatherInfo.humidity);
  tft.print("%");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 175);
  tft.print("Wind:");
  tft.setCursor(10, 195);
  tft.print(weatherInfo.wind, 1);
  tft.print(" km/h");

  // Updated time
  tft.setTextSize(1);
  tft.setTextColor(DARKGREY);
  tft.setCursor(10, 220);
  tft.print("Updated: ");
  tft.print(weatherInfo.time);
}
