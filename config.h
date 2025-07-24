#ifndef CONFIG_H
#define CONFIG_H

// Anzahl der Ebenen (Strips) - must be compile-time constant for array sizes
#define NUM_LEVELS 7

// Anzahl LEDs pro Strip
extern const int stripNumLEDs[];

// Abstand in cm pro LED für jeden Strip
extern const float stripLEDdistances[];

// GPIO Pins für jeden Strip
extern const int stripPins[];

// WLAN-Zugangsdaten (SSIDs und Passwörter)
extern const char* wifi_ssids[];
extern const char* wifi_passwords[];
extern const int wifi_count;

// MQTT-Server und Zugangsdaten
extern const char* mqtt_server;
extern const char* mqtt_username;
extern const char* mqtt_password;

// OTA (Over-the-Air Update) Einstellungen
extern const char* ota_hostname;
extern const char* ota_password;

#endif
