#ifndef DISPLAY_WRAPPER_H
#define DISPLAY_WRAPPER_H
#include <Wire.h>
#include "../config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
extern Adafruit_SSD1306 display;
const int G_COLOR_WHITE = SSD1306_WHITE;
const int G_COLOR_BLACK = SSD1306_BLACK;
inline void g_init_display() {
    // Pass 'false' as 4th argument (periphBegin) to prevent the library 
    // from calling Wire.begin() and resetting our custom I2C pins (18/8).
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false)) {
        Serial.println(F("SSD1306 allocation failed"));
    }
}
inline void g_clear_display() { display.clearDisplay(); }
inline void g_update_display() { display.display(); }
inline void g_draw_filled_round_rect(int x, int y, int w, int h, int r, int color) {
    display.fillRoundRect(x, y, w, h, r, color);
}
inline void g_draw_filled_triangle(int x0, int y0, int x1, int y1, int x2, int y2, int color) {
    display.fillTriangle(x0, y0, x1, y1, x2, y2, color);
}
#endif