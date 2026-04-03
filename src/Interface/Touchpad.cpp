#include "Touchpad.h"
#include <Arduino.h>

#define NUM_TOUCH_PINS 4

// Définition des pins capacitives (Touch1..4)
static const int touchPins[NUM_TOUCH_PINS] = {4, 5, 6, 7};

// Variables internes
static int baselines[NUM_TOUCH_PINS] = {0};
static int currentValues[NUM_TOUCH_PINS] = {0};
static int minValues[NUM_TOUCH_PINS] = {0};
static int maxValues[NUM_TOUCH_PINS] = {0};
static bool touchStates[NUM_TOUCH_PINS] = {false};
static bool anyTouchDetected = false;

/** Detection threshold as percentage of baseline increase */
static const float TOUCH_THRESHOLD_PERCENT = 0.30f;

namespace Drivers {
    namespace Touchpad {

        void init() {
            // Initial calibration: establish baseline through multi-sample averaging
            for (int i = 0; i < NUM_TOUCH_PINS; i++) {
                minValues[i] = 100000;
                maxValues[i] = 0;
                long sum = 0;
                const int samples = 20;
                for (int j = 0; j < samples; j++) {
                    sum += touchRead(touchPins[i]);
                    delay(2);
                }
                baselines[i] = sum / samples;
                Serial.printf("INFO: Touchpad %d (IO%d) Baseline: %d\n", i+1, touchPins[i], baselines[i]);
            }
        }

        void update() {
            for (int i = 0; i < NUM_TOUCH_PINS; i++) {
                int val = touchRead(touchPins[i]);
                currentValues[i] = val;

                // Mise à jour des extremes observés
                if (val < minValues[i]) minValues[i] = val;
                if (val > maxValues[i]) maxValues[i] = val;

                int threshold = baselines[i] * (1.0f + TOUCH_THRESHOLD_PERCENT);
                
                if (val > threshold) {
                    touchStates[i] = true;
                    anyTouchDetected = true; 
                } else {
                    touchStates[i] = false;
                    
                    // Drift compensation algorithm
                    // Adjust baseline towards current idle value using a low-pass filter
                    if (val < threshold) { 
                         baselines[i] = (int)((baselines[i] * 0.995) + (val * 0.005));
                    }
                }
            }
        }

        bool isTouched(uint8_t index) {
            if (index >= NUM_TOUCH_PINS) return false;
            return touchStates[index];
        }

        int getValue(uint8_t index) {
            if (index >= NUM_TOUCH_PINS) return 0;
            return currentValues[index];
        }

        float getStrength(uint8_t index) {
            if (index >= NUM_TOUCH_PINS) return 0.0f;
            int val = currentValues[index];
            int base = baselines[index];
            int maxVal = maxValues[index];

            int diff = val - base;
            if (diff < 0) diff = 0; // Ignore les valeurs sous la baseline (bruit négatif)

            int range = maxVal - base;
            // Imposer une plage minimale (~20000) pour éviter que le bruit n'amplifie la jauge au démarrage
            if (range < 20000) range = 20000;

            float strength = (float)diff / (float)range;
            if (strength > 1.0f) strength = 1.0f;
            return strength;
        }

        int getMin(uint8_t index) {
            if (index >= NUM_TOUCH_PINS) return 0;
            return minValues[index];
        }

        int getMax(uint8_t index) {
            if (index >= NUM_TOUCH_PINS) return 0;
            return maxValues[index];
        }

        bool isDetected() {
            return anyTouchDetected;
        }
    }
}
