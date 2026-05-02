#include "LCD_Internal.h"
#include <LittleFS.h>
#include <Arduino.h>

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

        if (*targetBuffer) {
            free(*targetBuffer);
            *targetBuffer = nullptr;
        }
        *targetLoaded = false;

        File file = LittleFS.open(filename, "r");
        if (!file) {
            Serial.printf("setAnimation: fichier introuvable %s\n", filename);
            return;
        }

        size_t fSize = file.size();
        if (fSize < 480) {
            Serial.printf("setAnimation: fichier trop petit %s (%u o)\n", filename, (unsigned)fSize);
            file.close();
            return;
        }

        *targetBuffer = (uint16_t*)ps_malloc(fSize);
        if (!*targetBuffer) {
            Serial.printf("setAnimation: ps_malloc echec %s (%u o)\n", filename, (unsigned)fSize);
            file.close();
            return;
        }

        size_t rd = file.read((uint8_t*)*targetBuffer, fSize);
        file.close();
        if (rd != fSize) {
            Serial.printf("setAnimation: lecture incomplete %s\n", filename);
            free(*targetBuffer);
            *targetBuffer = nullptr;
            return;
        }

        if (fSize % 480 != 0) {
            Serial.printf("WARN: setAnimation %s taille %u pas multiple de 480 (240×2) → stride strip douteux\n",
                          filename, (unsigned)fSize);
        }
        *targetWidth = (abs((long)fSize - 576000) < 4096) ? 1200 : (int)(fSize / 480);
        if (*targetWidth < 240) *targetWidth = 240;
        Serial.printf("INFO: setAnimation %s bytes=%u sheetWidthPx=%d\n", filename, (unsigned)fSize, *targetWidth);
        *targetLoaded = true;
    }

    void loadRobotEyeRes(const char* filename) {
        _robotEyeLoaded = false;
        if (_robotEyeBuffer) {
            free(_robotEyeBuffer);
            _robotEyeBuffer = nullptr;
        }
        File file = LittleFS.open(filename, "r");
        if (!file || file.size() != 245000) {
            if (file) file.close();
            Serial.printf("loadRobotEyeRes: fichier absent ou mauvaise taille (%s, attendu 245000 octets)\n",
                          filename ? filename : "?");
            return;
        }
        _robotEyeBuffer = (uint16_t*)ps_malloc(245000);
        if (_robotEyeBuffer) {
            size_t rd = file.read((uint8_t*)_robotEyeBuffer, 245000);
            file.close();
            if (rd == 245000) {
                _robotEyeLoaded = true;
                Serial.printf("loadRobotEyeRes: OK %s\n", filename);
            } else {
                Serial.println("loadRobotEyeRes: lecture incomplete");
                free(_robotEyeBuffer);
                _robotEyeBuffer = nullptr;
            }
        } else {
            file.close();
            Serial.println("loadRobotEyeRes: ps_malloc echoue");
        }
    }

    bool _drawFaceToSprite(LGFX_Sprite& sprite, uint16_t* buffer, bool loaded, int sheetWidth, int index) {
        if (!loaded || !buffer || !sprite.getBuffer()) return false;
        const int face_w = 240;
        int cols = sheetWidth / face_w;
        if (cols < 1) cols = 1;
        int horizontalOffset = ((index % cols) + cols) % cols * face_w;
        uint16_t* spriteBuffer = (uint16_t*)sprite.getBuffer();

        for (int y = 0; y < 240; y++) {
            memcpy(spriteBuffer + (y * 240), buffer + (y * sheetWidth) + horizontalOffset, 480);
        }
        return true;
    }

    bool isRobotEyeResourceReady() { return _robotEyeLoaded; }

    bool haveCatStripAtlas() { return _animLeftLoaded || _animRightLoaded; }

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