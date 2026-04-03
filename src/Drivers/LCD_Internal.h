#ifndef DRIVERS_LCD_INTERNAL_H
#define DRIVERS_LCD_INTERNAL_H

#include "LCD.h"

namespace Drivers {
    // Objets globaux partagés
    extern LGFX_Sprite _sprite_right;
    extern LGFX_Sprite _sprite_left;
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

    // Performance
    extern unsigned long time_draw_gmeter_total;
    extern unsigned long time_push_gmeter_total;
    extern unsigned long time_draw_horizon_total;
    extern unsigned long time_push_horizon_total;
    extern int frame_count_for_timing;

    // Helpers internes
    void _pushDMA(LGFX_Sprite& sprite, lgfx::LGFX_Device& tft);
    void _drawFaceToSprite(LGFX_Sprite& sprite, uint16_t* buffer, bool loaded, int sheetWidth, int index);
    void _fillEyeBuffer(uint16_t* destBuffer, float normX, float normY);
    void _drawTofDebugGridToBuffer(uint16_t* destBuffer, const uint16_t* grid);
}
#endif