#include "OLED.h"
#include "LCD.h"
#include "Interface/display_wrapper.h"
#include "Core/EngineState.h"
#include <Arduino.h>

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

namespace Drivers {

    static bool _oledReady = false;

    static void bumpOledFrameCounter() {
        ENGINE_STATE.oledFrameCounter.fetch_add(1, std::memory_order_relaxed);
    }

    void initOLED() {
        g_init_display();

        display.ssd1306_command(0xD5);
        display.ssd1306_command(0xF0);

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 10);
        display.println(F("Engine Started"));
        display.display();

        _oledReady = true;
        Serial.println(F("Drivers: OLED Adafruit Initialise"));
    }

    void updateOLED(float roll, float pitch, bool btConnected, const float* touchStrengths) {
        if (!_oledReady) return;

        display.clearDisplay();

        if (touchStrengths) {
            const int bar_w = 20;
            const int bar_h = 42;
            const int gap = 10;
            const int start_x = (SCREEN_WIDTH - (4 * bar_w + 3 * gap)) / 2;
            const int start_y = 4;

            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);

            for (int i = 0; i < 4; i++) {
                int x_pos = start_x + i * (bar_w + gap);
                float strength = touchStrengths[i];
                if (strength < 0.0f) strength = 0.0f;
                if (strength > 1.0f) strength = 1.0f;

                int fill_height = (int)(strength * bar_h);

                display.drawRect(x_pos, start_y, bar_w, bar_h, SSD1306_WHITE);

                if (fill_height > 0) {
                    display.fillRect(x_pos, start_y + (bar_h - fill_height), bar_w, fill_height, SSD1306_WHITE);
                }

                display.setCursor(x_pos + 4, start_y + bar_h + 6);
                display.print("P");
                display.print(i + 1);
            }
        }

        display.display();
        bumpOledFrameCounter();
    }

    void updateOLEDPerformance() {
        if (!_oledReady) return;

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);

        const float fps = ENGINE_STATE.perfDbgFps.load();
        const uint32_t hf = ENGINE_STATE.perfHeapFree.load();
        const uint32_t hm = ENGINE_STATE.perfHeapMin.load();
        const uint32_t ps = ENGINE_STATE.perfPsramFree.load();
        const uint32_t up = ENGINE_STATE.perfUptimeSec.load();
        const bool bt = ENGINE_STATE.btConnected.load();
        const int face = ENGINE_STATE.activeFaceId.load();

        char line[22];

        snprintf(line, sizeof(line), "FPS:%.1f  Up:%lus", (double)fps, (unsigned long)up);
        display.println(line);

        snprintf(line, sizeof(line), "SR %luk m%luk",
                 (unsigned long)(hf / 1024u), (unsigned long)(hm / 1024u));
        display.println(line);

#if defined(BOARD_HAS_PSRAM)
        snprintf(line, sizeof(line), "PSR %luk", (unsigned long)(ps / 1024u));
#else
        snprintf(line, sizeof(line), "PSR --");
#endif
        display.println(line);

        snprintf(line, sizeof(line), "BT %s  lcd %d", bt ? "*" : "-", face);
        display.println(line);

        const bool strip = haveCatStripAtlas();
        const bool robo = isRobotEyeResourceReady();
        snprintf(line, sizeof(line), "atlas%s robo%s", strip ? "+" : "-", robo ? "+" : "-");
        display.println(line);

        display.display();
        bumpOledFrameCounter();
    }
}
