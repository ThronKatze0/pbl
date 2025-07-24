#include "qr_scanner.h"
#include "mqtt_handler.h"
#include <Wire.h>

M5UnitQRCodeI2C qrcode;
String lastQRCode = "";
unsigned long lastQRCodeTime = 0;

void initializeQRScanner() {
  Wire.begin(9, 8);
  logMessage("Initializing QR code scanner...");
  while (!qrcode.begin(&Wire, UNIT_QRCODE_ADDR, 21, 22, 100000U)) {
    logMessage("Unit QRCode I2C Init Fail, retrying...");
    delay(1000);
  }
  logMessage("Unit QRCode I2C Init Success");
  qrcode.setTriggerMode(AUTO_SCAN_MODE);
}

void handleQRScanning() {
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
