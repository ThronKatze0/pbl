#include "config.h"

// Anzahl LEDs pro Strip
const int stripNumLEDs[NUM_LEVELS] = {38, 38, 38, 38, 38, 38, 38};

// Abstand in cm pro LED für jeden Strip
const float stripLEDdistances[NUM_LEVELS] = {3.3, 3.3, 3.3, 3.3, 3.3, 3.3, 3.3};

// GPIO Pins für jeden Strip
const int stripPins[NUM_LEVELS] = {7, 1, 6, 37, 35, 36, 5};

// WLAN-Zugangsdaten (SSIDs und Passwörter)
const char* wifi_ssids[] = {
  "smart",
  "Ziethenfunk II",
  "Ziethenfunk I",
  "Ziethenfunk IV",
  "Ziethenfunk V"
};

const char* wifi_passwords[] = {
  "smartinnovativ",
  "Nussing2991",
  "Nussing2991",
  "Nussing2991",
  "Nussing2991"
};

const int wifi_count = sizeof(wifi_ssids) / sizeof(wifi_ssids[0]);

// MQTT-Server und Zugangsdaten
const char* mqtt_server = "10.0.0.29";
const char* mqtt_username = "m5stack";
const char* mqtt_password = "c=Yu#DkuaL3}g^@";

// OTA (Over-the-Air Update) Einstellungen
const char* ota_hostname = "SmartPBL";
const char* ota_password = "innovativsmart";
