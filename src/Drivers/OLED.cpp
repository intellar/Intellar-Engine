#include "OLED.h"
#include "Interface/display_wrapper.h" // Inclut U8g2 et les helpers
#include <Arduino.h>

// Instanciation de l'objet U8g2 (Référencé via extern dans display_wrapper.h)
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

namespace Drivers {

    static bool _oledReady = false;

    void initOLED() {
        // Utilisation de la fonction commune d'initialisation
        // Elle force l'adresse I2C et appelle display.begin()
        g_init_display();
        
        // --- Optimisation pour la vidéo (Réduction du scintillement) ---
        // On règle l'oscillateur interne au maximum (0xF) et le diviseur au minimum (0x0)
        // Registre 0xD5 : Set Display Clock Divide Ratio/Oscillator Frequency
        display.ssd1306_command(0xD5); 
        display.ssd1306_command(0xF0); 

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 10);
        display.println(F("Engine Started"));
        display.display();
        
        _oledReady = true;
        Serial.println(F("Drivers: OLED Adafruit Initialise"));
    }

    void updateOLED(float roll, float pitch, bool btConnected, const float* touchStrengths) {
        if (!_oledReady) return; // Sécurité si non initialisé
        
        display.clearDisplay();

        // Affichage de 4 grands rectangles pour visualiser l'état des pads tactiles
        if (touchStrengths) {
            const int rect_width = 30;
            const int rect_height = 60;
            const int gap = 2;
            const int start_x = (SCREEN_WIDTH - (4 * rect_width + 3 * gap)) / 2; // Centrage du bloc
            const int start_y = (SCREEN_HEIGHT - rect_height) / 2;

            for (int i = 0; i < 4; i++) {
                int x_pos = start_x + i * (rect_width + gap);
                float strength = touchStrengths[i];
                if (strength < 0.0f) strength = 0.0f;
                if (strength > 1.0f) strength = 1.0f;

                int fill_height = (int)(strength * rect_height);
                
                // Dessin du cadre (Hollow rectangle) - Adapte pour Adafruit
                display.drawRect(x_pos, start_y, rect_width, rect_height, SSD1306_WHITE);
                
                if (fill_height > 0) {
                    // Remplissage depuis le bas (Filled rectangle) - Adapte pour Adafruit
                    display.fillRect(x_pos, start_y + (rect_height - fill_height), rect_width, fill_height, SSD1306_WHITE);
                }
            }
        }

        display.display();
    }
}