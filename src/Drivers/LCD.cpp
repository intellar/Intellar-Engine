#include "LCD_Internal.h"
#include "Core/EngineState.h"

namespace Drivers {

    bool _showingFace = false;
    void drawArtificialHorizon(LGFX_Sprite& sprite, float roll, float pitch);
    void drawGmeter(LGFX_Sprite& sprite, float ax, float ay, float az);

    void _pushDMA(LGFX_Sprite& sprite, lgfx::LGFX_Device& tft) {
        tft.waitDMA();

        tft.startWrite();
        tft.pushImageDMA(0, 0, sprite.width(), sprite.height(), (lgfx::rgb565_t*)sprite.getBuffer());
        tft.endWrite();
    }

    void _finalizeDMA() {
        _tft_left.waitDMA();
        _tft_right.waitDMA();
    }

    void updateLCD() {
        if (!_isInitialized) return;

        // Sélection des buffers de travail (Back buffers)
        LGFX_Sprite& spriteL = _eyeSprites[LEFT_EYE][_backBufferIdx];
        LGFX_Sprite& spriteR = _eyeSprites[RIGHT_EYE][_backBufferIdx];

        float roll = ENGINE_STATE.imuRoll[0].load();
        float pitch = ENGINE_STATE.imuPitch[0].load();
        float ax = ENGINE_STATE.imuAccel[0][0];
        float ay = ENGINE_STATE.imuAccel[0][1];
        float az = ENGINE_STATE.imuAccel[0][2];

#if defined(SCREEN_GC9A01_DUAL)
        drawGmeter(spriteL, ax, ay, az);
        _pushDMA(spriteL, _tft_left);

        drawArtificialHorizon(spriteR, roll, pitch);
        _pushDMA(spriteR, _tft_right);

        _finalizeDMA();
        // Swap de l'index pour la prochaine frame
        _backBufferIdx = (_backBufferIdx == 0) ? 1 : 0;
#else
        static unsigned long lastTextUpdate = 0;
        if (millis() - lastTextUpdate > 100) {
            _tft.setTextColor(TFT_WHITE, TFT_BLACK);
            _tft.setTextSize(2);
            _tft.setCursor(10, 10);
            _tft.println("IMU Status");
            lastTextUpdate = millis();
        }
        drawArtificialHorizon(spriteR, roll, pitch);
#if defined(SCREEN_GC9A01)
        spriteR.pushSprite(120, 60);
#elif defined(SCREEN_ILI9341)
        spriteR.pushSprite(160, 120);
#endif
#endif
    }

    void reportTimings() { /* Timing logic removed, no action needed */ }

    void drawTouchMarker(int x, int y) { if (_isInitialized) _eyeSprites[RIGHT_EYE][_backBufferIdx].fillCircle(x, y, 3, TFT_YELLOW); }
    void displayTouchCoords(int x, int y) { }

    void showCatFace(int leftIndex, int rightIndex) {
        if (!_isInitialized) return;
        _showingFace = true;

        LGFX_Sprite& spriteL = _eyeSprites[LEFT_EYE][_backBufferIdx];
        LGFX_Sprite& spriteR = _eyeSprites[RIGHT_EYE][_backBufferIdx];

#if defined(SCREEN_GC9A01_DUAL)
        _drawFaceToSprite(spriteL, _animBufferLeft, _animLeftLoaded, _widthLeft, leftIndex);
        _pushDMA(spriteL, _tft_left);

        _drawFaceToSprite(spriteR, _animBufferRight, _animRightLoaded, _widthRight, rightIndex);
        _pushDMA(spriteR, _tft_right);
        _finalizeDMA();
        _backBufferIdx = (_backBufferIdx == 0) ? 1 : 0;
#else
        _drawFaceToSprite(spriteR, _animBufferLeft, _animLeftLoaded, _widthLeft, leftIndex);
        spriteR.pushSprite(0, 0);
#endif
    }

    void showRobotEyes(float normX, float normY, const uint16_t* grid) {
        if (!_isInitialized) return;
        _showingFace = true;
        
        // 1. On choisit les buffers "Back" (ceux où on va écrire)
        LGFX_Sprite& back_left = _eyeSprites[LEFT_EYE][_backBufferIdx];
        LGFX_Sprite& back_right = _eyeSprites[RIGHT_EYE][_backBufferIdx];

#if defined(SCREEN_GC9A01_DUAL)
        // 2. COPIE CPU (memcpy rapide)
        // On remplit les buffers qui ne sont pas en train d'être envoyés
        _fillEyeBuffer((uint16_t*)back_left.getBuffer(), normX, normY);
        _fillEyeBuffer((uint16_t*)back_right.getBuffer(), normX, normY);

        if (grid) {
            _drawTofDebugGridToBuffer((uint16_t*)back_right.getBuffer(), grid);
        }

        // 3. ENVOI DMA (Asynchrone)
        // _pushDMA appellera waitDMA() en interne pour s'assurer que le bus est libre
        _pushDMA(back_left, _tft_left);
        _pushDMA(back_right, _tft_right);
        _finalizeDMA();
        _backBufferIdx = (_backBufferIdx == 0) ? 1 : 0;
#else
        back_right.setSwapBytes(false);
        _fillEyeBuffer((uint16_t*)back_right.getBuffer(), normX, normY);
        if (grid) _drawTofDebugGridToBuffer((uint16_t*)back_right.getBuffer(), grid);
        back_right.pushSprite(0, 0);
        _backBufferIdx = (_backBufferIdx == 0) ? 1 : 0;
#endif
    }
}