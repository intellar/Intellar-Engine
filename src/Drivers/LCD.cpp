#include "LCD_Internal.h"
#include "Core/EngineState.h"
#include <algorithm>

namespace Drivers {

    bool _showingFace = false;
    void drawArtificialHorizon(LGFX_Sprite& sprite, float roll, float pitch);
    void drawGmeter(LGFX_Sprite& sprite, float ax, float ay, float az);

#if defined(SCREEN_GC9A01_DUAL)
    static void _flushSpriteBlocking(LGFX_Sprite& sprite, lgfx::LGFX_Device& tft) {
        tft.waitDMA();
        tft.startWrite();
        tft.pushImage(0, 0, sprite.width(), sprite.height(),
                     reinterpret_cast<const lgfx::rgb565_t*>(sprite.getBuffer()));
        tft.endWrite();
    }
#endif

    void _pushDMA(LGFX_Sprite& sprite, lgfx::LGFX_Device& tft) {
        tft.waitDMA();
#if defined(SCREEN_GC9A01_DUAL)
        _flushSpriteBlocking(sprite, tft);
#else
        tft.startWrite();
        tft.pushImageDMA(0, 0, sprite.width(), sprite.height(), (lgfx::rgb565_t*)sprite.getBuffer());
        tft.endWrite();
#endif
    }

    void _finalizeDMA() {
        _tft_left.waitDMA();
        _tft_right.waitDMA();
    }

    void updateLCD() {
        if (!_isInitialized) return;

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
        _backBufferIdx = (_backBufferIdx == 0) ? 1 : 0;
#else
        static unsigned long lastTextUpdate = 0;
        if (millis() - lastTextUpdate > 100) {
            _tft_right.setTextColor(TFT_WHITE, TFT_BLACK);
            _tft_right.setTextSize(2);
            _tft_right.setCursor(10, 10);
            _tft_right.println("IMU Status");
            lastTextUpdate = millis();
        }
        drawArtificialHorizon(spriteR, roll, pitch);
#if defined(SCREEN_GC9A01)
        spriteR.pushSprite(120, 60);
#elif defined(SCREEN_ILI9341)
        /** ILI9347 paysage 320×240 : centrer un sprite 240×240. */
        spriteR.pushSprite(&_tft_right, 40, 0);
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
        uint16_t* bufL = _animBufferLeft;
        bool okL = _animLeftLoaded;
        int wL = _widthLeft;
        if (!okL && _animRightLoaded && _animBufferRight) {
            bufL = _animBufferRight;
            okL = true;
            wL = _widthRight;
        }

        uint16_t* bufR = _animBufferRight;
        bool okR = _animRightLoaded;
        int wR = _widthRight;
        if (!okR && _animLeftLoaded && _animBufferLeft) {
            bufR = _animBufferLeft;
            okR = true;
            wR = _widthLeft;
        }

        _drawFaceToSprite(spriteL, bufL, okL, wL, leftIndex);
        _pushDMA(spriteL, _tft_left);

        _drawFaceToSprite(spriteR, bufR, okR, wR, rightIndex);
        _pushDMA(spriteR, _tft_right);
        _finalizeDMA();
        _backBufferIdx = (_backBufferIdx == 0) ? 1 : 0;
#else
        _drawFaceToSprite(spriteR, _animBufferLeft, _animLeftLoaded, _widthLeft, leftIndex);
#if defined(SCREEN_ILI9341)
        spriteR.pushSprite(&_tft_right, 40, 0);
#else
        spriteR.pushSprite(0, 0);
#endif
#endif
    }

    void showRobotEyes(float normX, float normY, const uint16_t* grid) {
        if (!_isInitialized) return;
        _showingFace = true;

        LGFX_Sprite& back_left = _eyeSprites[LEFT_EYE][_backBufferIdx];
        LGFX_Sprite& back_right = _eyeSprites[RIGHT_EYE][_backBufferIdx];

#if defined(SCREEN_GC9A01_DUAL)
        _fillEyeBuffer((uint16_t*)back_left.getBuffer(), normX, normY);
        _fillEyeBuffer((uint16_t*)back_right.getBuffer(), normX, normY);

        if (grid) {
            _drawTofDebugGridToBuffer((uint16_t*)back_right.getBuffer(), grid);
        }

        _pushDMA(back_left, _tft_left);
        _pushDMA(back_right, _tft_right);
        _finalizeDMA();
        _backBufferIdx = (_backBufferIdx == 0) ? 1 : 0;
#else
        back_right.setSwapBytes(false);
        _fillEyeBuffer((uint16_t*)back_right.getBuffer(), normX, normY);
        if (grid) _drawTofDebugGridToBuffer((uint16_t*)back_right.getBuffer(), grid);
#if defined(SCREEN_ILI9341)
        back_right.pushSprite(&_tft_right, 40, 0);
#else
        back_right.pushSprite(0, 0);
#endif
        _backBufferIdx = (_backBufferIdx == 0) ? 1 : 0;
#endif
    }

#if defined(SCREEN_ILI9341) && defined(TOUCH_CS)

    /** Mapping XPT2046 → pixels écran. Sur-corriger via build_flags si besoin (voir platformio.ini ili9341). */
    static inline void mapXptPointToScreen(const lgfx::LGFX_Device& d, int32_t& x, int32_t& y) {
        const int32_t w = d.width();
        const int32_t h = d.height();
        if (w <= 0 || h <= 0) return;
#ifdef ILI9341_TOUCH_SWAP_XY
        std::swap(x, y);
#endif
#if defined(ILI9341_TOUCH_FIX_NONE)
#elif defined(ILI9341_TOUCH_FIX_MIRROR_X)
        x = w - 1 - x;
#elif defined(ILI9341_TOUCH_FIX_MIRROR_Y)
        y = h - 1 - y;
#else
        /** Shield Intellar ILI9341 + XPT2046 : défaut LovyanGFX + miroir X+Y nécessaires. */
        x = w - 1 - x;
        y = h - 1 - y;
#endif
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= w) x = w - 1;
        if (y >= h) y = h - 1;
    }

    /** Coordonnées stylet prêtes pour l’UI – même correction que le feedback jaune. */
    bool getIli9341TouchScreenPos(int16_t* outX, int16_t* outY) {
        if (!_isInitialized || !tftTouchSubsystemReady() || !outX || !outY) return false;
        lgfx::touch_point_t tp;
        if (getTFT().getTouch(&tp, 1) == 0) return false;
        int32_t x = tp.x;
        int32_t y = tp.y;
        mapXptPointToScreen(getTFT(), x, y);
        *outX = (int16_t)x;
        *outY = (int16_t)y;
        return true;
    }

    void drawTftTouchFeedback() {
        if (!_isInitialized || !tftTouchSubsystemReady()) return;
        lgfx::touch_point_t tp[2];
        const uint_fast8_t n = getTFT().getTouch(tp, 2);
        if (n == 0) return;
        lgfx::LGFX_Device& d = getTFT();
#ifdef ILI9341_TOUCH_DEBUG
        for (uint_fast8_t i = 0; i < n; i++) {
            Serial.printf("touch raw map: library x=%d y=%d  wh=%d,%d\n", (int)tp[i].x, (int)tp[i].y,
                          (int)d.width(), (int)d.height());
        }
#endif
        d.startWrite();
        for (uint_fast8_t i = 0; i < n; i++) {
            int32_t x = tp[i].x;
            int32_t y = tp[i].y;
            mapXptPointToScreen(d, x, y);
#ifdef ILI9341_TOUCH_DEBUG
            Serial.printf("touch after map: x=%d y=%d\n", (int)x, (int)y);
#endif
            d.fillCircle((int)x, (int)y, 5, TFT_YELLOW);
        }
        d.endWrite();
    }
#endif
}
