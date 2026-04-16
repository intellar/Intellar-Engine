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

        // Affichage de 4 bandes verticales pour visualiser la force du touchpad
        if (touchStrengths) {
            const int bar_w = 20;      // Largeur de chaque bande
            const int bar_h = 42;      // Hauteur maximale de la bande (ajusté pour l'écran 64px)
            const int gap = 10;        // Espace entre les bandes
            const int start_x = (SCREEN_WIDTH - (4 * bar_w + 3 * gap)) / 2; // Centrage horizontal
            const int start_y = 4;     // Marge haute

            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);

            for (int i = 0; i < 4; i++) {
                int x_pos = start_x + i * (bar_w + gap);
                float strength = touchStrengths[i];
                if (strength < 0.0f) strength = 0.0f;
                if (strength > 1.0f) strength = 1.0f;

                int fill_height = (int)(strength * bar_h);
                
                // Dessin du contour de la bande
                display.drawRect(x_pos, start_y, bar_w, bar_h, SSD1306_WHITE);
                
                // Remplissage progressif du bas vers le haut
                if (fill_height > 0) {
                    display.fillRect(x_pos, start_y + (bar_h - fill_height), bar_w, fill_height, SSD1306_WHITE);
                }

                // Ajout d'une étiquette P1, P2... sous chaque barre
                display.setCursor(x_pos + 4, start_y + bar_h + 6);
                display.print("P");
                display.print(i + 1);
            }
        }

        display.display();
    }
}