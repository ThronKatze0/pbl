#include <Adafruit_NeoPixel.h>
#include <M5Unified.h>
#include <Wire.h>
#include <M5GFX.h>
#include "M5UnitQRCode.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define PIN         37
#define NUM_LEDS    150
#define LED_DISTANCE_CM 3.7

Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

M5UnitQRCodeI2C qrcode;
M5Canvas canvas(&M5.Display);

const char* ssid = "Ziethenfunk II";
const char* password = "Nussing2991";
const char* mqtt_server = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = String((char*)payload);

  Serial.printf("Received message on topic %s: %s\n", topic, message.c_str());

  if (String(topic) == "pbl/light") {
    int sep = message.indexOf(':');
    if (sep == -1) return;

    float startCM = message.substring(0, sep).toFloat();
    float endCM = message.substring(sep + 1).toFloat();

    int startLED = round(startCM / LED_DISTANCE_CM);
    int endLED = round(endCM / LED_DISTANCE_CM);

    startLED = constrain(startLED, 0, NUM_LEDS - 1);
    endLED = constrain(endLED, 0, NUM_LEDS - 1);

    if (startLED > endLED) std::swap(startLED, endLED);

    for (int i = 0; i < NUM_LEDS; i++) {
      if (i >= startLED && i <= endLED) {
        strip.setPixelColor(i, strip.Color(255, 0, 0));  // red
      } else {
        strip.setPixelColor(i, 0);  // off
      }
    }
    strip.show();
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
    if (client.connect("M5StackClient")) {
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

void setup() {
  M5.begin();
  Serial.begin(9600);
  strip.begin();
  strip.show();

  canvas.setColorDepth(1);
  canvas.createSprite(M5.Display.width(), M5.Display.height());
  canvas.setTextSize((float)canvas.width() / 160);
  canvas.setTextColor(WHITE);
  canvas.fillSprite(BLACK);
  canvas.setTextScroll(true);
  canvas.pushSprite(0, 0);

  Wire.begin(9, 8);
  while (!qrcode.begin(&Wire, UNIT_QRCODE_ADDR, 21, 22, 100000U)) {
    canvas.println("Unit QRCode I2C Init Fail");
    Serial.println("Unit QRCode I2C Init Fail");
    canvas.pushSprite(0, 0);
    delay(1000);
  }
  Serial.println("Unit QRCode I2C Init Success");
  canvas.println("QRCode Init OK");
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

  if (qrcode.getDecodeReadyStatus() == 1) {
    uint8_t buffer[512] = {0};
    uint16_t length = qrcode.getDecodeLength();
    qrcode.getDecodeData(buffer, length);
    buffer[length] = '\0';

    String qrString = String((char*)buffer);
    Serial.printf("QR: %s\n", qrString.c_str());
    canvas.println(qrString);
    canvas.pushSprite(0, 0);

    client.publish("pbl/qr", qrString.c_str());
  }
}
