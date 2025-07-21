#include <NeoPixelBus.h>
#include <TFT_eSPI.h>  // Include TFT library

// Number of LEDs per strip
const uint16_t numLeds = 150;

// Define the NeoPixelBus for each strip
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip1(numLeds, 17);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip2(numLeds, 5);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip3(numLeds, 6);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip4(numLeds, 7);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip5(numLeds, 1);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip6(numLeds, 2);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip7(numLeds, 14);

uint16_t hue = 0;            // Current hue (0–360)
const uint16_t hueMax = 360; // Maximum hue value
const uint16_t hueDelta = 8; // Hue increment per loop (larger = faster)

TFT_eSPI tft = TFT_eSPI();  // Create TFT instance

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Initializing...");

  // Initialize the TFT display WITHOUT M5.begin()
  tft.init();
  tft.setRotation(1); // Rotate screen if needed
  tft.fillScreen(TFT_BLACK);

  // Display "Hello World"
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawString("Hello World", 10, 40);

  strip1.Begin();
  strip2.Begin();
  strip3.Begin();
  strip4.Begin();
  strip5.Begin();
  strip6.Begin();
  strip7.Begin();

  Serial.println("Running...");
}

void loop() {
  for (uint16_t i = 0; i < numLeds; i++) {
    uint16_t ledHue = (hue + (i * 360 / numLeds)) % hueMax;
    HslColor color(ledHue / (float)hueMax, 1.0f, 0.5f);

    strip1.SetPixelColor(i, color);
    strip2.SetPixelColor(i, color);
    strip3.SetPixelColor(i, color);
    strip4.SetPixelColor(i, color);
    strip5.SetPixelColor(i, color);
    strip6.SetPixelColor(i, color);
    strip7.SetPixelColor(i, color);
  }

  strip1.Show();
  strip2.Show();
  strip3.Show();
  strip4.Show();
  strip5.Show();
  strip6.Show();
  strip7.Show();

  hue = (hue + hueDelta) % hueMax;

  delay(20);
}
