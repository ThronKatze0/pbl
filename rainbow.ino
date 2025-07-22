#include <NeoPixelBus.h>
#include <M5Unified.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

const char* wifi_ssids[] = {"smart", "Ziethenfunk II", "Ziethenfunk I", "Ziethenfunk IV", "Ziethenfunk V"};
const char* wifi_passwords[] = {"smartinnovativ", "Nussing2991", "Nussing2991", "Nussing2991", "Nussing2991"};
const int wifi_count = sizeof(wifi_ssids) / sizeof(wifi_ssids[0]);

const uint16_t numLeds = 150;
const int totalStrips = 7;
const int totalLeds = totalStrips * numLeds;

NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip1(numLeds, 37);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip2(numLeds, 5);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip3(numLeds, 6);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip4(numLeds, 7);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip5(numLeds, 1);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip6(numLeds, 36);
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip7(numLeds, 35);

NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>* strips[] = {
  &strip1, &strip2, &strip3, &strip4, &strip5, &strip6, &strip7
};

const int racerCount = 20;
int racerPositions[racerCount];
int racerSpeeds[racerCount];

void setup_wifi() {
  for (int i = 0; i < wifi_count; i++) {
    WiFi.begin(wifi_ssids[i], wifi_passwords[i]);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
      delay(500);
      retries++;
    }
    if (WiFi.status() == WL_CONNECTED) return;
  }
  delay(5000);
  ESP.restart();
}

void setup() {
  auto cfg = M5.config();
  cfg.external_speaker.module_display = true;
  cfg.output_power = false;
  M5.begin(cfg);
  M5.Power.setExtOutput(false);
  M5.Speaker.begin();
  auto spk = M5.Speaker.config();
  spk.dma_buf_len = 256;
  spk.dma_buf_count = 8;
  M5.Speaker.config(spk);
  M5.Speaker.setVolume(128);
  setup_wifi();
  M5.Speaker.tone(6000, 50);

  for (int i = 0; i < totalStrips; i++) {
    strips[i]->Begin();
    strips[i]->ClearTo(RgbColor(0, 0, 0));
    strips[i]->Show();
  }

  for (int i = 0; i < racerCount; i++) {
    racerPositions[i] = (i * totalLeds) / racerCount;
    racerSpeeds[i] = random(1, 4); // speeds between 1 and 3
  }

  randomSeed(esp_random());

  ArduinoOTA.setHostname("SmartPBL");
  ArduinoOTA.setPassword("innovativsmart");
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();

  for (int s = 0; s < totalStrips; s++) {
    for (int i = 0; i < numLeds; i++) {
      strips[s]->SetPixelColor(i, RgbColor(0, 0, 0));
    }
  }

  for (int r = 0; r < racerCount; r++) {
    int pos = racerPositions[r];
    int stripNum = pos / numLeds;
    int ledIndex = pos % numLeds;
    if (stripNum < totalStrips) {
      strips[stripNum]->SetPixelColor(ledIndex, RgbColor(255, 255, 255));
    }

    racerPositions[r] = (racerPositions[r] + racerSpeeds[r]) % totalLeds;

    if (random(0, 100) < 3) {
      racerSpeeds[r] = random(1, 4); // occasionally change speed
    }
  }

  for (int i = 0; i < totalStrips; i++) {
    strips[i]->Show();
  }

  delay(20);
}
