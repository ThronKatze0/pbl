#ifndef LED_STRIPS_H
#define LED_STRIPS_H

#include <NeoPixelBus.h>
#include <config.h>

// Forward declaration
void logMessage(const char* format, ...);

// External strip objects
extern NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strips[NUM_LEVELS];

// Rainbow mode variables
extern bool rainbowMode;
extern uint16_t rainbowStep;
extern unsigned long previousRainbowMillis;
extern const long rainbowInterval;

// Racer mode variables
extern bool racerMode;
extern uint16_t racerStep;
extern unsigned long previousRacerMillis;
extern const long racerInterval;
extern const int tailLength;
extern float racerSpeed[NUM_LEVELS];
extern float racerPosFloat[NUM_LEVELS];
extern float speedPhase[NUM_LEVELS];
extern int racerPositions[NUM_LEVELS];
extern int racerDirections[NUM_LEVELS];
extern float racerHue[NUM_LEVELS];
extern unsigned long racerStartTime;

// Active level tracking
extern int activeLevel;
extern float activeStartCM;
extern float activeEndCM;
extern unsigned long activeLevelStartTime;
extern unsigned long previousPulseMillis;
extern const long pulseInterval;
extern bool pulseModeActive;

// Function declarations
void initializeStrips();
void rainbowCycle();
void setupSpeeds();
void racerCycle();
void setLevelRange(int level, float startCM, float endCM);
void updatePulsingLevel();

#endif
