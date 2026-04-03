#include "LCD_Internal.h"

namespace Drivers {
    void drawArtificialHorizon(LGFX_Sprite& sprite, float roll, float pitch) {
        int w = sprite.width(), h = sprite.height();
        int cx = w / 2, cy = h / 2;
        float tanRoll = tan(roll);
        int yOffset = pitch * 100;
        int y0 = cy + yOffset + (cx * tanRoll);
        int y1 = cy + yOffset - (cx * tanRoll);

        sprite.fillSprite(TFT_NAVY);
        sprite.fillTriangle(0, y0, w, y1, w, h, TFT_MAROON);
        sprite.fillTriangle(0, y0, 0, h, w, h, TFT_MAROON);
        sprite.drawFastHLine(cx - 10, cy, 20, TFT_WHITE);
        sprite.drawFastVLine(cx, cy - 10, 20, TFT_WHITE);
        sprite.drawCircle(cx, cy, cx - 1, TFT_WHITE);
    }

    void drawGmeter(LGFX_Sprite& sprite, float ax, float ay, float az) {
        int w = sprite.width(), h = sprite.height();
        int cx = w / 2, cy = h / 2;
        sprite.fillSprite(TFT_BLACK);
        sprite.drawCircle(cx, cy, cx - 10, TFT_DARKGREY);
        sprite.drawFastHLine(10, cy, w - 20, TFT_DARKGREY);
        sprite.drawFastVLine(cx, 10, h - 20, TFT_DARKGREY);
        sprite.drawCircle(cx, cy, cx - 1, TFT_WHITE);
        int radius = cx - 10;
        int gx = constrain(cx + (ax / 2.0f) * radius, 10, w - 10);
        int gy = constrain(cy - (ay / 2.0f) * radius, 10, h - 10);
        sprite.fillCircle(gx, gy, 8, TFT_RED);
        sprite.drawCircle(gx, gy, 8, TFT_WHITE);
    }

    void _drawTofDebugGridToBuffer(uint16_t* destBuffer, const uint16_t* grid) {
        if (!grid || !destBuffer) return;
        const int grid_size = 80;
        const int x_start = 80, y_start = 80;
        const float cell_size = 10.0f;

        for (int gy = 0; gy < 8; gy++) {
            for (int gx = 0; gx < 8; gx++) {
                int idx = (7 - gy) * 8 + gx;
                uint16_t dist = grid[idx];
                
                float intensity = 0.0f;
                if (dist > 30 && dist < 2000) {
                    // Luminosity mapping: inverse intensity based on proximity
                    intensity = 1.0f - constrain((float)(dist - 100) / 1500.0f, 0.0f, 1.0f);
                }

                uint8_t gray = (uint8_t)(intensity * 255);
                uint16_t color = ((gray >> 3) << 11) | ((gray >> 2) << 5) | (gray >> 3);
                color = (color << 8) | (color >> 8);

                int cx_s = x_start + (int)(gx * cell_size);
                int cy_s = y_start + (int)(gy * cell_size);
                for (int py = cy_s; py < cy_s + 10; py++) {
                    for (int px = cx_s; px < cx_s + 10; px++) {
                        destBuffer[py * 240 + px] = color;
                    }
                }
            }
        }
    }
}