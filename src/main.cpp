#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// For the encoder, we will use interrupts to read the values
#define ENC_A 2
#define ENC_B 3
#define ENC_BTN 5
#define KO_BTN 1

// For the display, we will use the following pins:
#define TFT_SCLK 4
#define TFT_MOSI 6
#define TFT_RST 7
#define TFT_DC 8
#define TFT_CS 9
#define TFT_BLK 10

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);

bool lastBtnState = HIGH;
bool lastKoState = HIGH;
int currentMenu = 0;
volatile int encoderValue = 0;
volatile int lastEncoded = 0;
unsigned long lastMove = 0;
int incrementValue = 0;
enum Screen
{
  SCREEN_HOME = 0,
  SCREEN_INCREMENT = 1,
  SCREEN_SIMPLE = 2
};
Screen currentScreen = SCREEN_HOME;

void drawMenu();
void drawHomeMenu();
void drawIncrementMenu();
void drawSimpleMenu();
void IRAM_ATTR readEncoder();

void drawMenu()
{
  if (currentScreen == SCREEN_HOME)
    drawHomeMenu();
  else if (currentScreen == SCREEN_INCREMENT)
    drawIncrementMenu();
  else
    drawSimpleMenu();
}

void drawHomeMenu()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(15, 15);
  tft.print("Home");

  int y1 = 80;
  int y2 = 140;

  tft.setTextColor(currentMenu == 0 ? ST77XX_CYAN : ST77XX_WHITE);
  tft.drawRect(15, y1 - 10, 210, 40, currentMenu == 0 ? ST77XX_CYAN : ST77XX_WHITE);
  tft.setCursor(30, y1);
  tft.print("Increment");

  tft.setTextColor(currentMenu == 1 ? ST77XX_CYAN : ST77XX_WHITE);
  tft.drawRect(15, y2 - 10, 210, 40, currentMenu == 1 ? ST77XX_CYAN : ST77XX_WHITE);
  tft.setCursor(30, y2);
  tft.print("Simple");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 210);
  tft.print("Rotate to select, press to open");
}

void drawIncrementMenu()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(15, 15);
  tft.print("Increment");

  tft.setTextSize(3);
  tft.setCursor(15, 80);
  tft.setTextColor(ST77XX_CYAN);
  tft.print(incrementValue);
  tft.setTextColor(ST77XX_WHITE);

  tft.setTextSize(1);
  tft.setCursor(15, 190);
  tft.print("Press ENC to +1");

  tft.setCursor(15, 210);
  tft.print("Press KO to Home");
}

void drawSimpleMenu()
{
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(15, 15);
  tft.print("Simple");

  tft.setTextSize(3);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(15, 90);
  tft.print("Hello");
  tft.setCursor(15, 125);
  tft.print("World");
  tft.setTextColor(ST77XX_WHITE);

  tft.setTextSize(1);
  tft.setCursor(15, 210);
  tft.print("Press KO to Home");
}

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

  drawMenu();
}

void loop()
{
  static int lastEnc = 0;
  int val = encoderValue;
  int diff = val - lastEnc;
  if (diff > 0)
    diff = 1;
  if (diff < 0)
    diff = -1;

  if (currentScreen == SCREEN_HOME && val != lastEnc && millis() - lastMove > 120)
  {
    lastMove = millis();
    currentMenu = (currentMenu + diff + 2) % 2;
    drawMenu();
  }
  lastEnc = val;

  bool btnState = digitalRead(ENC_BTN);
  if (lastBtnState == HIGH && btnState == LOW)
  {
    if (currentScreen == SCREEN_HOME)
    {
      if (currentMenu == 0)
        currentScreen = SCREEN_INCREMENT;
      else
        currentScreen = SCREEN_SIMPLE;
      drawMenu();
    }
    else
    {
      if (currentScreen == SCREEN_INCREMENT)
      {
        incrementValue++;
        drawMenu();
      }
    }
  }
  lastBtnState = btnState;

  bool koState = digitalRead(KO_BTN);
  if (lastKoState == HIGH && koState == LOW)
  {
    if (currentScreen != SCREEN_HOME)
    {
      currentScreen = SCREEN_HOME;
      drawMenu();
    }
  }
  lastKoState = koState;

  delay(30);
}

// Encoder reading function
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
