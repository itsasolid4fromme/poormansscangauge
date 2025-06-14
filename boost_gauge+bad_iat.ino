
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int MAP_PIN = A0;   // Boost pressure signal
const int IAT_PIN = A1;   // IAT signal
const int OIL_PIN = A2;   // Oil pressure signal

const float OIL_V_MIN = 0.5;
const float OIL_V_MAX = 4.5;
const float OIL_PSI_MAX = 100.0;

unsigned long lastPeakUpdate = 0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 500;
const unsigned long peakResetInterval = 30000;

float peakBoost = 0.0;

// Convert IAT ADC reading to Fahrenheit using Steinhart-Hart for Bosch 0281006059 with 2k pull-up
float adcToTempF(int adc) {
  float V = adc * (5.0 / 1023.0);
  float Rsensor = (5.0 - V) * 2000.0 / V; // 2k pull-up resistor
  float lnR = log(Rsensor);
  float invT = 0.001305213816 + 0.0002582304849 * lnR + 0.0000001769345582 * pow(lnR, 3);
  float Tkelvin = 1.0 / invT;
  float Tc = Tkelvin - 273.15;
  return Tc * 9.0 / 5.0 + 32.0;  // Fahrenheit
}

void setup() {
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println("BOOST GAUGE");
  display.display();
  delay(1500);
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate < updateInterval) return;
  lastUpdate = now;

  // BOOST PSI
  float vBoost = analogRead(MAP_PIN) * 5.0 / 1023.0;
  float kPa = ((vBoost - 0.5) / 4.0) * 400.0;
  float boostPSI = (kPa * 0.145038) - 14.7;
  if (boostPSI < 0) boostPSI = 0;

  if (boostPSI > peakBoost) {
    peakBoost = boostPSI;
    lastPeakUpdate = now;
  }
  if (now - lastPeakUpdate >= peakResetInterval) {
    peakBoost = 0;
  }

  // IAT
  int iatADC = analogRead(IAT_PIN);
  float tempF = adcToTempF(iatADC);

  // OIL
  float vOil = analogRead(OIL_PIN) * 5.0 / 1023.0;
  float oilPSI = max(0.0f, ((vOil - OIL_V_MIN) / (OIL_V_MAX - OIL_V_MIN)) * OIL_PSI_MAX);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("BST ");
  display.print((int)boostPSI);
  display.setCursor(80, 0);
  display.print("PK");
  display.print((int)peakBoost);

  display.setCursor(0, 22);
  display.print("OIL ");
  display.print((int)oilPSI);

  display.setCursor(0, 44);
  display.print("IAT ");
  display.print((int)tempF);
  display.print((char)247);
  display.print("F");

  display.display();
}
