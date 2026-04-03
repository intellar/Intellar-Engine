#include "LCD_Internal.h"
#include <LittleFS.h>

namespace Drivers {
    uint16_t* _animBufferLeft = nullptr;
    uint16_t* _animBufferRight = nullptr;
    bool _animLeftLoaded = false;
    bool _animRightLoaded = false;
    int _widthLeft = 1200;
    int _widthRight = 1200;
    uint16_t* _robotEyeBuffer = nullptr;
    bool _robotEyeLoaded = false;

    void setAnimation(const char* filename, DisplayIndex display) {
        uint16_t** targetBuffer = (display == DisplayIndex::LEFT) ? &_animBufferLeft : &_animBufferRight;
        bool* targetLoaded = (display == DisplayIndex::LEFT) ? &_animLeftLoaded : &_animRightLoaded;
        int* targetWidth = (display == DisplayIndex::LEFT) ? &_widthLeft : &_widthRight;

        if (*targetBuffer) free(*targetBuffer);
        
        File file = LittleFS.open(filename, "r");
        if (file) {
            size_t fSize = file.size();
            *targetBuffer = (uint16_t*)ps_malloc(fSize);
            if (*targetBuffer) {
                file.read((uint8_t*)*targetBuffer, fSize);
            // Perform byte swapping for binary compatibility with display driver
            for (size_t i = 0; i < fSize / 2; i++) {
                (*targetBuffer)[i] = __builtin_bswap16((*targetBuffer)[i]);
            }
            *targetWidth = (abs((long)fSize - 576000) < 4096) ? 1200 : (fSize / 480);
                *targetLoaded = true;
            }
            file.close();
        }
    }

    void loadRobotEyeRes(const char* filename) {
        if (_robotEyeBuffer) free(_robotEyeBuffer);
        File file = LittleFS.open(filename, "r");
        if (file && file.size() == 245000) {
            _robotEyeBuffer = (uint16_t*)ps_malloc(245000);
            if (_robotEyeBuffer) {
                file.read((uint8_t*)_robotEyeBuffer, 245000);
                for (size_t i = 0; i < 122500; i++) {
                    _robotEyeBuffer[i] = __builtin_bswap16(_robotEyeBuffer[i]);
                }
                _robotEyeLoaded = true;
            }
            file.close();
        }
    }

    void _drawFaceToSprite(LGFX_Sprite& sprite, uint16_t* buffer, bool loaded, int sheetWidth, int index) {
        if (!loaded || !buffer || !sprite.getBuffer()) return;
        const int face_w = 240;
        int horizontalOffset = (index % (sheetWidth / face_w)) * face_w;
        uint16_t* spriteBuffer = (uint16_t*)sprite.getBuffer();

        for (int y = 0; y < 240; y++) {
            memcpy(spriteBuffer + (y * 240), buffer + (y * sheetWidth) + horizontalOffset, 480);
        }
    }

    void _fillEyeBuffer(uint16_t* destBuffer, float normX, float normY) {
        if (!_robotEyeLoaded || !_robotEyeBuffer || !destBuffer) return;
        const int srcW = 350;
        const int destW = 240;
        const int destH = 240;

        int startX = constrain(55 + (int)(normX * 55.0f), 0, srcW - destW);
        int startY = constrain(55 + (int)(normY * 55.0f), 0, srcW - destH);
        // Serial.printf("_fillEyeBuffer: normX=%.2f, normY=%.2f, startX=%d, startY=%d\n", normX, normY, startX, startY); // DEBUG: Vérifier les valeurs

        for (int y = 0; y < destH; y++) {
            const Scanline& line = _circularScanlines[y];
            int x_start = line.x_start;
            int width = line.x_end - x_start;
            if (width > 0) {
                memcpy(destBuffer + (y * destW) + x_start, 
                       _robotEyeBuffer + ((startY + y) * srcW) + startX + x_start, 
                       width * 2);
            }
        }
    }
}