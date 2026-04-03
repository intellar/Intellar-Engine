#ifndef DRIVERS_LCD_INTERNAL_H
#define DRIVERS_LCD_INTERNAL_H

#include "LCD.h"

namespace Drivers {
    #define LEFT_EYE 0
    #define RIGHT_EYE 1

    // Objets globaux partagés
    extern LGFX_Sprite _eyeSprites[2][2];
    extern int _backBufferIdx;
    extern bool _isInitialized;
    extern bool _showingFace;

    // Buffers d'animation
    extern uint16_t* _animBufferLeft;
    extern uint16_t* _animBufferRight;
    extern bool _animLeftLoaded;
    extern bool _animRightLoaded;
    extern int _widthLeft;
    extern int _widthRight;

    // Robot Eye
    struct Scanline { int16_t x_start; int16_t x_end; };
    extern Scanline _circularScanlines[240];
    extern uint16_t* _robotEyeBuffer;
    extern bool _robotEyeLoaded;

    // Helpers internes
    void _pushDMA(LGFX_Sprite& sprite, lgfx::LGFX_Device& tft);
    void _drawFaceToSprite(LGFX_Sprite& sprite, uint16_t* buffer, bool loaded, int sheetWidth, int index);
    void _fillEyeBuffer(uint16_t* destBuffer, float normX, float normY);
    void _drawTofDebugGridToBuffer(uint16_t* destBuffer, const uint16_t* grid);
}
#endif