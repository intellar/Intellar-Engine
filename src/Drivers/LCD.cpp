#include "LCD_Internal.h"
#include "Core/EngineState.h"

namespace Drivers {

    bool _showingFace = false;
    void drawArtificialHorizon(LGFX_Sprite& sprite, float roll, float pitch);
    void drawGmeter(LGFX_Sprite& sprite, float ax, float ay, float az);

    void _pushDMA(LGFX_Sprite& sprite, lgfx::LGFX_Device& tft) {
        // Ensure the shared SPI bus is available by waiting for pending DMA transfers
        // across both instances before performing a Chip Select (CS) switch.
        _tft_left.waitDMA();
        _tft_right.waitDMA();
        
        // Manual CS management via startWrite/endWrite is omitted to prevent early 
        // CS high transitions during asynchronous DMA operations.
        sprite.pushSprite(&tft, 0, 0); 
    }

    void _finalizeDMA() {
        _tft_left.waitDMA();
        _tft_right.waitDMA();
    }

    void updateLCD() {
        if (!_isInitialized) return;

        float roll = ENGINE_STATE.imuRoll;
        float pitch = ENGINE_STATE.imuPitch;
        float ax = ENGINE_STATE.imuAccel[0];
        float ay = ENGINE_STATE.imuAccel[1];
        float az = ENGINE_STATE.imuAccel[2];

#if defined(SCREEN_GC9A01_DUAL)
        drawGmeter(_sprite_left, ax, ay, az);
        _pushDMA(_sprite_left, _tft_left);

        drawArtificialHorizon(_sprite_right, roll, pitch);
        _pushDMA(_sprite_right, _tft_right);

        _finalizeDMA();
#else
        static unsigned long lastTextUpdate = 0;
        if (millis() - lastTextUpdate > 100) {
            _tft.setTextColor(TFT_WHITE, TFT_BLACK);
            _tft.setTextSize(2);
            _tft.setCursor(10, 10);
            _tft.println("IMU Status");
            lastTextUpdate = millis();
        }
        drawArtificialHorizon(_sprite_right, roll, pitch);
#if defined(SCREEN_GC9A01)
        _sprite_right.pushSprite(120, 60);
#elif defined(SCREEN_ILI9341)
        _sprite_right.pushSprite(160, 120);
#endif
#endif
    }

    void reportTimings() { }

    void drawTouchMarker(int x, int y) { if (_isInitialized) _sprite_right.fillCircle(x, y, 3, TFT_YELLOW); }
    void displayTouchCoords(int x, int y) { }

    void showCatFace(int leftIndex, int rightIndex) {
        if (!_isInitialized) return;
        _showingFace = true;

#if defined(SCREEN_GC9A01_DUAL)
        _drawFaceToSprite(_sprite_left, _animBufferLeft, _animLeftLoaded, _widthLeft, leftIndex);
        _pushDMA(_sprite_left, _tft_left);

        _drawFaceToSprite(_sprite_right, _animBufferRight, _animRightLoaded, _widthRight, rightIndex);
        _pushDMA(_sprite_right, _tft_right);
        _finalizeDMA();
#else
        _drawFaceToSprite(_sprite_right, _animBufferLeft, _animLeftLoaded, _widthLeft, leftIndex);
        _sprite_right.pushSprite(0, 0);
#endif
    }

    void showRobotEyes(float normX, float normY, const uint16_t* grid) {
        if (!_isInitialized) return;
        _showingFace = true;

#if defined(SCREEN_GC9A01_DUAL)
        // 1. Prépare le buffer gauche
        _fillEyeBuffer((uint16_t*)_sprite_left.getBuffer(), normX, normY);
        // 2. Lance l'envoi du buffer gauche en arrière-plan (DMA)
        _pushDMA(_sprite_left, _tft_left);

        // 3. PENDANT que le DMA travaille sur l'écran gauche, le CPU calcule l'oeil droit
        _fillEyeBuffer((uint16_t*)_sprite_right.getBuffer(), normX, normY);
        // On n'affiche la grille de debug que sur l'œil droit (HUD)
        if (grid) _drawTofDebugGridToBuffer((uint16_t*)_sprite_right.getBuffer(), grid);
        // 4. Envoie le buffer droit (attendra automatiquement que le bus SPI soit libre)
        _pushDMA(_sprite_right, _tft_right);
        _finalizeDMA();
#else
        _sprite_right.setSwapBytes(false);
        _fillEyeBuffer((uint16_t*)_sprite_right.getBuffer(), normX, normY);
        if (grid) _drawTofDebugGridToBuffer((uint16_t*)_sprite_right.getBuffer(), grid);
        _sprite_right.pushSprite(0, 0);
#endif
    }
}