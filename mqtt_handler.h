#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <PubSubClient.h>
#include <WiFiClient.h>

// External MQTT client objects
extern WiFiClient espClient;
extern PubSubClient client;

// Function declarations
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setup_wifi();
void reconnect();
void logMessage(const char* format, ...);

#endif
