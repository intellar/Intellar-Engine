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
    /** Affiche un framebuffer 240×240 RGB565 (Little Endian) sur un œil GC9A01 / TFT droit. */
    void pushVideo565(DisplayIndex which, const uint16_t* rgb565);
    void loadRobotEyeRes(const char* filename); // Charge l'image de l'œil géant
    void showRobotEyes(float normX, float normY, const uint16_t* grid = nullptr); // Dessine l'œil avec décalage + debug ToF
    /** `image_giant.bin` présent avec la taille attendue pour showRobotEyes */
    bool isRobotEyeResourceReady();
    /** Au moins un strip chat chargé depuis LittleFS / BLE */
    bool haveCatStripAtlas();
    void reportTimings();
    
    // Access to the raw object if needed for custom drawing
    lgfx::LGFX_Device& getTFT();

    /** Tactile TFT (SPI, ex. XPT2046) — uniquement avec SCREEN_ILI9341 et TOUCH_CS en build_flags. */
    bool tftTouchSubsystemReady();
#if defined(SCREEN_ILI9341) && defined(TOUCH_CS)
    /** Point jaune sous le doigt / stylet (debug tactile). */
    void drawTftTouchFeedback();
    /** Stylet corrigé (miroir XPT / écran) — pour future UI tactile. */
    bool getIli9341TouchScreenPos(int16_t* outX, int16_t* outY);
#endif
}

#endif