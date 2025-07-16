#include <FastLED.h>
#include <M5Unified.h>
#include <Wire.h>
#include <M5GFX.h>
#include "M5UnitQRCode.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define NUM_LEVELS 1
#define MAX_LEDS 38

// Strip configuration
const int stripNumLEDs[NUM_LEVELS] =        {38};
const float stripLEDdistances[NUM_LEVELS] = {3.3};

// Pins must be compile-time constants
#define STRIP0_PIN 6
#define STRIP1_PIN 7

CRGB leds0[MAX_LEDS];
CRGB leds1[MAX_LEDS];
CRGB* ledLevels[NUM_LEVELS] = {leds0};

M5UnitQRCodeI2C qrcode;

// WiFi + MQTT
const char* ssid = "smart";
const char* password = "smartinnovativ";
const char* mqtt_server = "10.0.0.29";
const char* mqtt_username = "m5stack";
const char* mqtt_password = "c=Yu#DkuaL3}g^@";

WiFiClient espClient;
PubSubClient client(espClient);

bool rainbowMode = false;
uint16_t rainbowStep = 0;

unsigned long previousRainbowMillis = 0;
const long rainbowInterval = 2;

CRGB* getStrip(int level) {
  return ledLevels[level];
}

void showStrip(int level) {
  FastLED.show();
}

// Wheel function like Adafruit's
CRGB Wheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return CRGB(255 - pos * 3, 0, pos * 3);
  } else if (pos < 170) {
    pos -= 85;
    return CRGB(0, pos * 3, 255 - pos * 3);
  } else {
    pos -= 170;
    return CRGB(pos * 3, 255 - pos * 3, 0);
  }
}

void rainbowCycle() {
  for (int l = 0; l < NUM_LEVELS; l++) {
    int numLEDs = stripNumLEDs[l];
    CRGB* strip = getStrip(l);
    for (int i = 0; i < numLEDs; i++) {
      int pixelIndex = (i * 256 / numLEDs) + rainbowStep;
      strip[i] = Wheel(pixelIndex & 255);
    }
  }
  FastLED.show();
  rainbowStep++;
  if (rainbowStep >= 256) rainbowStep = 0;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = String((char*)payload);

  Serial.printf("Received message on topic %s: %s\n", topic, message.c_str());

  if (String(topic) == "pbl/light") {
    if (message == "rainbow") {
      rainbowMode = true;
      return;
    }

    int firstSep = message.indexOf(':');
    int secondSep = message.indexOf(':', firstSep + 1);
    if (firstSep == -1 || secondSep == -1) return;

    int level = message.substring(0, firstSep).toInt() - 1;
    float startCM = message.substring(firstSep + 1, secondSep).toFloat();
    float endCM = message.substring(secondSep + 1).toFloat();

    if (level < 0 || level >= NUM_LEVELS) {
      Serial.println("Invalid level");
      return;
    }

    rainbowMode = false;

    int startLED = round(startCM / stripLEDdistances[level]);
    int endLED = round(endCM / stripLEDdistances[level]);

    startLED = constrain(startLED, 0, stripNumLEDs[level] - 1);
    endLED = constrain(endLED, 0, stripNumLEDs[level] - 1);
    if (startLED > endLED) std::swap(startLED, endLED);

    // Turn off all other strips
    for (int l = 0; l < NUM_LEVELS; l++) {
      if (l != level) {
        for (int i = 0; i < stripNumLEDs[l]; i++) {
          ledLevels[l][i] = CRGB::Black;
        }
      }
    }

    for (int i = 0; i < stripNumLEDs[level]; i++) {
      if (i >= startLED && i <= endLED) {
        ledLevels[level][i] = CRGB::Blue;
      } else {
        ledLevels[level][i] = CRGB::Black;
      }
    }

    FastLED.show();
  }
}

void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("M5StackClient", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("pbl/light");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
    Serial.println();
  }
}

void setupStrips() {
  for (int i = 0; i < NUM_LEVELS; i++) {
    switch (i) {
      case 0:
        FastLED.addLeds<WS2812B, STRIP0_PIN, GRB>(leds0, stripNumLEDs[0]);
        break;
      case 1:
        FastLED.addLeds<WS2812B, STRIP1_PIN, GRB>(leds1, stripNumLEDs[1]);
        break;
    }
  }
  FastLED.clear(true);
}

void setup() {
  M5.begin();
  Serial.begin(9600);

  setupStrips();

  Wire.begin(9, 8);
  while (!qrcode.begin(&Wire, UNIT_QRCODE_ADDR, 21, 22, 100000U)) {
    Serial.println("Unit QRCode I2C Init Fail");
    delay(1000);
  }
  Serial.println("Unit QRCode I2C Init Success");
  qrcode.setTriggerMode(AUTO_SCAN_MODE);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
}

void loop() {
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
    Serial.printf("len:%d\r\n", length);
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
        char c = static_cast<char>(buffer[i]);
        code += c;
      }
    }

    Serial.println(code);
    client.publish("pbl/qr", code.c_str());
  }
}
