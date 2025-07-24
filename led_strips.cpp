#include "led_strips.h"
#include "mqtt_handler.h"

// Strip objects
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strips[NUM_LEVELS] = {
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[0], stripPins[0]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[1], stripPins[1]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[2], stripPins[2]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[3], stripPins[3]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[4], stripPins[4]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[5], stripPins[5]),
  NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod>(stripNumLEDs[6], stripPins[6])
};

// Rainbow mode variables
bool rainbowMode = false;
uint16_t rainbowStep = 0;
unsigned long previousRainbowMillis = 0;
const long rainbowInterval = 3;

// Racer mode variables
bool racerMode = false;
uint16_t racerStep = 0;
unsigned long previousRacerMillis = 0;
const long racerInterval = 25;  // Smooth but not too fast
const int tailLength = 3;       // Nice visible tail
float racerSpeed[NUM_LEVELS];
float racerPosFloat[NUM_LEVELS];
float speedPhase[NUM_LEVELS];
int racerPositions[NUM_LEVELS] = {0};
int racerDirections[NUM_LEVELS] = {1};
float racerHue[NUM_LEVELS];     // Individual hue for each strip
unsigned long racerStartTime = 0;

// Active level tracking
int activeLevel = -1;
float activeStartCM = 0.0;
float activeEndCM = 0.0;
unsigned long activeLevelStartTime = 0;
unsigned long previousPulseMillis = 0;
const long pulseInterval = 30; // Update pulse every 30ms for smooth animation
bool pulseModeActive = false;

void initializeStrips() {
  for (int i = 0; i < NUM_LEVELS; i++) {
    strips[i].Begin();
    strips[i].Show();
    logMessage("Initialized strip %d on pin %d with %d LEDs", i + 1, stripPins[i], stripNumLEDs[i]);
  }
}

void rainbowCycle() {
  for (int l = 0; l < NUM_LEVELS; l++) {
    int numLEDs = stripNumLEDs[l];
    for (int i = 0; i < numLEDs; i++) {
      int pixelIndex = (i * 256 / numLEDs) + rainbowStep;
      HslColor color(((pixelIndex & 255) / 255.0f), 1.0f, 0.5f);
      strips[l].SetPixelColor(i, color);
    }
    strips[l].Show();
  }
  rainbowStep++;
  if (rainbowStep >= 256) rainbowStep = 0;
}

void setupSpeeds() {
  racerStartTime = millis();
  for (int l = 0; l < NUM_LEVELS; l++) {
    speedPhase[l] = random(0, 628) / 100.0f;
    racerSpeed[l] = random(80, 250) / 100.0f;  // Reasonable racing speeds
    racerPosFloat[l] = random(0, stripNumLEDs[l]);
    racerDirections[l] = random(0, 2) * 2 - 1; // -1 or 1
    racerHue[l] = random(0, 360);              // Random starting hue
  }
}

void racerCycle() {
  for (int l = 0; l < NUM_LEVELS; l++) {
    // Gradual speed variations for natural racing feel
    racerSpeed[l] += (random(-5, 6) / 100.0f);
    
    // Keep speeds in racing range
    if (racerSpeed[l] < 0.5f) racerSpeed[l] = 0.5f;
    if (racerSpeed[l] > 3.0f) racerSpeed[l] = 3.0f;

    racerPosFloat[l] += racerSpeed[l] * racerDirections[l];
    int numLEDs = stripNumLEDs[l];

    // Bounce at ends
    if (racerPosFloat[l] >= numLEDs - 1) {
      racerPosFloat[l] = numLEDs - 1;
      racerDirections[l] = -1;
    } else if (racerPosFloat[l] <= 0) {
      racerPosFloat[l] = 0;
      racerDirections[l] = 1;
    }

    int pos = (int)racerPosFloat[l];
    
    // Slowly shift hue for each racer
    racerHue[l] += 0.5f;
    if (racerHue[l] > 360.0f) racerHue[l] -= 360.0f;
    
    // Clear the strip
    for (int i = 0; i < numLEDs; i++) {
      strips[l].SetPixelColor(i, RgbColor(0, 0, 0));
    }
    
    // Draw the racer with tail
    for (int i = 0; i < numLEDs; i++) {
      int dist = abs(i - pos);
      
      if (dist == 0) {
        // Main racer - bright and clear
        HsbColor mainColor(racerHue[l] / 360.0f, 1.0f, 1.0f);
        strips[l].SetPixelColor(i, mainColor);
      } else if (dist <= tailLength) {
        // Tail with smooth fade
        float tailBrightness = 1.0f - ((float)dist / (tailLength + 1));
        HsbColor tailColor(racerHue[l] / 360.0f, 1.0f, tailBrightness * 0.7f);
        strips[l].SetPixelColor(i, tailColor);
      }
    }
    
    strips[l].Show();
  }
}

void setLevelRange(int level, float startCM, float endCM) {
  if (level < 0 || level >= NUM_LEVELS) {
    logMessage("Invalid level %d in range setting.", level + 1);
    return;
  }

  rainbowMode = false;
  racerMode = false;
  pulseModeActive = true;
  activeLevelStartTime = millis();
  previousPulseMillis = 0; // Reset pulse timing
  
  logMessage("Setting pulsing LEDs for level %d, range %.2f cm to %.2f cm", level + 1, startCM, endCM);
  logMessage("Pulse mode activated: %s", pulseModeActive ? "true" : "false");

  int startLED = round(startCM / stripLEDdistances[level]);
  int endLED = round(endCM / stripLEDdistances[level]);

  startLED = constrain(startLED, 0, stripNumLEDs[level] - 1);
  endLED = constrain(endLED, 0, stripNumLEDs[level] - 1);
  if (startLED > endLED) std::swap(startLED, endLED);

  logMessage("Constrained LED indices: start %d, end %d", startLED, endLED);

  // Turn off all other levels
  for (int l = 0; l < NUM_LEVELS; l++) {
    if (l != level) {
      for (int i = 0; i < stripNumLEDs[l]; i++) {
        strips[l].SetPixelColor(i, RgbColor(0,0,0));
      }
      strips[l].Show();
    }
  }

  activeLevel = level;
  activeStartCM = startCM;
  activeEndCM = endCM;
  
  // Initial update to show the range immediately
  updatePulsingLevel();
  
  logMessage("Updated strip %d LEDs from %d to %d with pulsing effect.", level + 1, startLED, endLED);
}

void updatePulsingLevel() {
  if (!pulseModeActive || activeLevel < 0) {
    return;
  }
  
  unsigned long currentTime = millis();
  float timePhase = (currentTime - activeLevelStartTime) / 1000.0f;
  
  int startLED = round(activeStartCM / stripLEDdistances[activeLevel]);
  int endLED = round(activeEndCM / stripLEDdistances[activeLevel]);
  
  startLED = constrain(startLED, 0, stripNumLEDs[activeLevel] - 1);
  endLED = constrain(endLED, 0, stripNumLEDs[activeLevel] - 1);
  if (startLED > endLED) std::swap(startLED, endLED);
  
  // Create a smooth breathing effect
  float breatheIntensity = (sin(timePhase * 1.5f) * 0.4f + 0.6f); // Oscillates between 0.2 and 1.0
  
  // Add subtle color shifting
  float hueShift = timePhase * 20.0f; // Slow hue rotation
  float baseHue = 240.0f + sin(timePhase * 0.3f) * 60.0f; // Blue with slight variation
  if (baseHue > 360.0f) baseHue -= 360.0f;
  if (baseHue < 0.0f) baseHue += 360.0f;
  
  // Clear the strip first
  for (int i = 0; i < stripNumLEDs[activeLevel]; i++) {
    strips[activeLevel].SetPixelColor(i, RgbColor(0, 0, 0));
  }
  
  // Draw the pulsing range with gradient effects
  for (int i = 0; i < stripNumLEDs[activeLevel]; i++) {
    if (i >= startLED && i <= endLED) {
      // Distance from center of the range for gradient effect
      float rangeCenter = (startLED + endLED) / 2.0f;
      float distFromCenter = abs(i - rangeCenter);
      float rangeLength = endLED - startLED + 1;
      
      // Create a gradient that's brightest in the center
      float gradientFactor = 1.0f - (distFromCenter / (rangeLength / 2.0f)) * 0.3f;
      gradientFactor = constrain(gradientFactor, 0.4f, 1.0f);
      
      // Add subtle sparkle effect
      float sparkle = 1.0f;
      if (random(0, 1000) < 5) { // 0.5% chance for sparkle
        sparkle = 1.3f;
      }
      
      // Calculate final intensity
      float finalIntensity = breatheIntensity * gradientFactor * sparkle;
      finalIntensity = constrain(finalIntensity, 0.0f, 1.0f);
      
      // Set color with subtle hue variation
      float pixelHue = baseHue + sin(i * 0.5f + timePhase * 2.0f) * 15.0f;
      if (pixelHue > 360.0f) pixelHue -= 360.0f;
      if (pixelHue < 0.0f) pixelHue += 360.0f;
      
      HsbColor pulseColor(pixelHue / 360.0f, 0.8f, finalIntensity);
      strips[activeLevel].SetPixelColor(i, pulseColor);
    }
  }
  
  strips[activeLevel].Show();
}
