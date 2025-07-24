#include <M5Unified.h>
#include "led_strips.h"
#include "mqtt_handler.h"
#include "qr_scanner.h"
#include "ota_handler.h"

M5Canvas canvas(&M5.Display);

void setup() {
  auto cfg = M5.config();
  cfg.external_speaker.module_display = true;
  M5.begin(cfg);
  M5.Speaker.begin();
  auto spk = M5.Speaker.config();
  spk.dma_buf_len   = 256;
  spk.dma_buf_count = 8;
  M5.Speaker.config(spk);
  M5.Speaker.setVolume(64);

  canvas.setColorDepth(1);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextSize((float)canvas.width() / 160);
  canvas.setTextScroll(true);

  canvas.println("Hello World!");
  canvas.pushSprite(0, 0);

  Serial.begin(115200);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  reconnect();

  setupOTA();
  
  initializeStrips();
  initializeQRScanner();

  logMessage("Setup complete.");
  M5.Speaker.tone(6000, 50);
}

void loop() {
  handleOTA();
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  unsigned long currentMillis = millis();
  if (rainbowMode) {
    if (currentMillis - previousRainbowMillis >= rainbowInterval) {
      previousRainbowMillis = currentMillis;
      rainbowCycle();
    }
  } else if (racerMode) {
    if (currentMillis - previousRacerMillis >= racerInterval) {
      previousRacerMillis = currentMillis;
      racerCycle();
    }
  } else if (pulseModeActive) {
    // Update pulsing effect with timing control for smooth animation
    if (currentMillis - previousPulseMillis >= pulseInterval) {
      previousPulseMillis = currentMillis;
      updatePulsingLevel();
    }
  }

  handleQRScanning();
}
