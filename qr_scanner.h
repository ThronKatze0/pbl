#ifndef QR_SCANNER_H
#define QR_SCANNER_H

#include "M5UnitQRCode.h"
#include <Arduino.h>

// External QR code scanner object
extern M5UnitQRCodeI2C qrcode;

// QR code tracking variables
extern String lastQRCode;
extern unsigned long lastQRCodeTime;

// Function declarations
void initializeQRScanner();
void handleQRScanning();

#endif
