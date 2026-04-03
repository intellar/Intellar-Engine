#ifndef DRIVERS_LCD_H
#define DRIVERS_LCD_H

#include <Arduino.h>
#include <LovyanGFX.hpp>

namespace Drivers {
    // Définitions de couleurs pour la compatibilité avec le code existant
    #define TFT_BLACK       0x0000
    #define TFT_NAVY        0x000F
    #define TFT_MAROON      0x7800
    #define TFT_DARKGREY    0x7BEF
    #define TFT_BLUE        0x001F
    #define TFT_GREEN       0x07E0
    #define TFT_CYAN        0x07FF
    #define TFT_RED         0xF800
    #define TFT_MAGENTA     0xF81F
    #define TFT_YELLOW      0xFFE0
    #define TFT_WHITE       0xFFFF
    #define TFT_ORANGE      0xFDA0
    #define TFT_PINK        0xFC9F

    enum class DisplayIndex { LEFT, RIGHT };

    // Exposition des instances LovyanGFX pour main.cpp
    extern lgfx::LGFX_Device& _tft_left;
    extern lgfx::LGFX_Device& _tft_right;

    void initLCD(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t led);
    void clearLCD();
    void drawTouchMarker(int x, int y);
    void displayTouchCoords(int x, int y);
    void setAnimation(const char* filename, DisplayIndex display = DisplayIndex::LEFT);
    void updateLCD();
    void showCatFace(int leftIndex, int rightIndex);
    void loadRobotEyeRes(const char* filename); // Charge l'image de l'œil géant
    void showRobotEyes(float normX, float normY, const uint16_t* grid = nullptr); // Dessine l'œil avec décalage + debug ToF
    void reportTimings();
    
    // Access to the raw object if needed for custom drawing
    lgfx::LGFX_Device& getTFT();
}

#endif