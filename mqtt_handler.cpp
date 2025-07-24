#include "mqtt_handler.h"
#include "led_strips.h"
#include <config.h>
#include <WiFi.h>

WiFiClient espClient;
PubSubClient client(espClient);

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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = String((char*)payload);

  if (String(topic) == "pbl/light") {
    if (message == "rainbow") {
      rainbowMode = true;
      racerMode = false;
      activeLevel = -1;
      logMessage("Rainbow mode activated.");
      return;
    } else if (message == "racer") {
      racerMode = true;
      rainbowMode = false;
      activeLevel = -1;
      setupSpeeds();
      logMessage("Racer mode activated");
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

    setLevelRange(level, startCM, endCM);
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
