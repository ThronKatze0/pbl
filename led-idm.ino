#include <NeoPixelBus.h>
#include <M5Unified.h>
#include <Wire.h>
#include "M5UnitQRCode.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

const int NUM_LEVELS = 7;

const int stripNumLEDs[NUM_LEVELS] = {38, 38, 38, 38, 38, 38, 38};
const float stripLEDdistances[NUM_LEVELS] = {3.3, 3.3, 3.3, 3.3, 3.3, 3.3, 3.3};
const int stripPins[NUM_LEVELS] = {7, 1, 6, 37, 35, 36, 5};

M5Canvas canvas(&M5.Display);

NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strips[NUM_LEVELS] = {
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[0], stripPins[0]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[1], stripPins[1]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[2], stripPins[2]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[3], stripPins[3]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[4], stripPins[4]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[5], stripPins[5]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[6], stripPins[6])
};

M5UnitQRCodeI2C qrcode;

const char* wifi_ssids[] = {"smart", "Ziethenfunk II", "Ziethenfunk I", "Ziethenfunk IV", "Ziethenfunk V"};
const char* wifi_passwords[] = {"smartinnovativ", "Nussing2991", "Nussing2991", "Nussing2991", "Nussing2991"};
const int wifi_count = sizeof(wifi_ssids) / sizeof(wifi_ssids[0]);
const char* mqtt_server = "10.0.0.29";
const char* mqtt_username = "m5stack";
const char* mqtt_password = "c=Yu#DkuaL3}g^@";

WiFiClient espClient;
PubSubClient client(espClient);

bool rainbowMode = false;
uint16_t rainbowStep = 0;
unsigned long previousRainbowMillis = 0;
const long rainbowInterval = 3;

String lastQRCode = "";
unsigned long lastQRCodeTime = 0;

int activeLevel = -1;
float activeStartCM = 0.0;
float activeEndCM = 0.0;

void logMessage(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  Serial.println(buffer);

  if (client.connected()) {
    client.publish("pbl/log", buffer);
  }
}

void rainbowCycle() {
  logMessage("Running rainbow cycle...");
  for (int l = 0; l < NUM_LEVELS; l++) {
    int numLEDs = stripNumLEDs[l];
    for (int i = 0; i < numLEDs; i++) {
      int pixelIndex = (i * 256 / numLEDs) + rainbowStep;
      HslColor color(((pixelIndex & 255) / 255.0f), 1.0f, 0.5f);
      strips[l].SetPixelColor(i, color);
    }
    strips[l].Show();
    logMessage("Rainbow updated strip %d with %d LEDs", l + 1, numLEDs);
  }
  rainbowStep++;
  if (rainbowStep >= 256) rainbowStep = 0;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("TOPIC!!!!!!: %s\n", topic);
  Serial.println(String(topic) == "pbl/light");
  payload[length] = '\0';
  String message = String((char*)payload);

  if (String(topic) == "pbl/light") {
    if (message == "rainbow") {
      rainbowMode = true;
      activeLevel = -1;
      logMessage("Rainbow mode activated.");
      return;
    }

    int firstSep = message.indexOf(':');
    int secondSep = message.indexOf(':', firstSep + 1);
    if (firstSep == -1 || secondSep == -1) {
      logMessage("MQTT message format invalid.");
      return;
    }

    int level = message.substring(0, firstSep).toInt() - 1;
    float startCM = message.substring(firstSep + 1, secondSep).toFloat();
    float endCM = message.substring(secondSep + 1).toFloat();

    if (level < 0 || level >= NUM_LEVELS) {
      logMessage("Invalid level %d in MQTT message.", level + 1);
      return;
    }

    rainbowMode = false;
    logMessage("Setting LEDs for level %d, range %.2f cm to %.2f cm", level + 1, startCM, endCM);

    int startLED = round(startCM / stripLEDdistances[level]);
    int endLED = round(endCM / stripLEDdistances[level]);

    startLED = constrain(startLED, 0, stripNumLEDs[level] - 1);
    endLED = constrain(endLED, 0, stripNumLEDs[level] - 1);
    if (startLED > endLED) std::swap(startLED, endLED);

    logMessage("Constrained LED indices: start %d, end %d", startLED, endLED);

    for (int l = 0; l < NUM_LEVELS; l++) {
      if (l != level) {
        for (int i = 0; i < stripNumLEDs[l]; i++) {
          strips[l].SetPixelColor(i, RgbColor(0,0,0));
        }
        strips[l].Show();
      }
    }

    for (int i = 0; i < stripNumLEDs[level]; i++) {
      if (i >= startLED && i <= endLED) {
        strips[level].SetPixelColor(i, RgbColor(0, 0, 255));
      } else {
        strips[level].SetPixelColor(i, RgbColor(0, 0, 0));
      }
    }
    strips[level].Show();
    logMessage("Updated strip %d LEDs from %d to %d to blue.", level + 1, startLED, endLED);

    activeLevel = level;
    activeStartCM = startCM;
    activeEndCM = endCM;
  }
}

void setup_wifi() {
  for (int i = 0; i < wifi_count; i++) {
    logMessage("Trying WiFi SSID: %s", wifi_ssids[i]);
    WiFi.begin(wifi_ssids[i], wifi_passwords[i]);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {
      delay(500);
      Serial.print(".");
      retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      logMessage("Connected to WiFi SSID: %s", wifi_ssids[i]);
      logMessage("IP address: %s", WiFi.localIP().toString().c_str());
      return;
    } else {
      logMessage("Failed to connect to %s", wifi_ssids[i]);
    }
  }

  logMessage("All WiFi connection attempts failed. Restarting...");
  delay(5000);
  ESP.restart();
}

void reconnect() {
  while (!client.connected()) {
    logMessage("Attempting MQTT connection...");
    if (client.connect("M5StackClient", mqtt_username, mqtt_password)) {
      logMessage("MQTT connected");
      client.subscribe("pbl/light");
      logMessage("Subscribed to pbl/light");
    } else {
      logMessage("MQTT connection failed, rc=%d. Retrying in 2s...", client.state());
      delay(2000);
    }
  }
}

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

  ArduinoOTA.setHostname("SmartPBL");
  ArduinoOTA.setPassword("innovativsmart");

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA update");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA update finished");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  
  for (int i = 0; i < NUM_LEVELS; i++) {
    strips[i].Begin();
    strips[i].Show();
    logMessage("Initialized strip %d on pin %d with %d LEDs", i + 1, stripPins[i], stripNumLEDs[i]);
  }

  Wire.begin(9, 8);
  logMessage("Initializing QR code scanner...");
  while (!qrcode.begin(&Wire, UNIT_QRCODE_ADDR, 21, 22, 100000U)) {
    logMessage("Unit QRCode I2C Init Fail, retrying...");
    delay(1000);
  }
  logMessage("Unit QRCode I2C Init Success");
  qrcode.setTriggerMode(AUTO_SCAN_MODE);

  logMessage("Setup complete.");

  M5.Speaker.tone(6000, 50);
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (rainbowMode) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousRainbowMillis >= rainbowInterval) {
      previousRainbowMillis = currentMillis;
      rainbowCycle();
    }
  }

  if (qrcode.getDecodeReadyStatus() == 1) {
    uint8_t buffer[512] = {0};
    uint16_t length = qrcode.getDecodeLength();
    String code = "";

    if (length == 65535) {
      qrcode.getDecodeData(buffer, 150);
      int semicolons = 4;
      for (int i = 0; i < 150; i++) {
        char c = static_cast<char>(buffer[i]);
        if (buffer[i] == ' ') continue;
        code += c;
        if (buffer[i] == ';') semicolons--;
        if (semicolons == 0) break;
      }
    } else {
      qrcode.getDecodeData(buffer, length);
      for (int i = 0; i < length; i++) {
        code += static_cast<char>(buffer[i]);
      }
    }

    logMessage("QR code scanned: %s", code.c_str());
    client.publish("pbl/qr", code.c_str());
    lastQRCode = code;
    lastQRCodeTime = millis();
  }
}
