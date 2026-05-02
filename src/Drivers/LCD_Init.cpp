#include "LCD_Internal.h"
#include <Arduino.h>

#if defined(SCREEN_ILI9341)
#include <lgfx/v1/panel/Panel_ILI9341.hpp>
#endif
#if defined(SCREEN_ILI9341) && defined(TOUCH_CS)
#include <lgfx/v1/touch/Touch_XPT2046.hpp>
#ifndef SPI_TOUCH_FREQUENCY
#define SPI_TOUCH_FREQUENCY 2500000
#endif
#endif

namespace Drivers {

#if defined(SCREEN_ILI9341)

    /** Une dalle tactile ILI9341 (SPI dédiée) — ne pas confondre avec les GC9A01 des yeux ronds. */
    class LGFX_ILI9341_Main : public lgfx::LGFX_Device {
        lgfx::Bus_SPI       _spi;
        lgfx::Panel_ILI9341 _panel;

    public:
        LGFX_ILI9341_Main() {
            {
                auto cfg = _spi.config();
                cfg.spi_host   = SPI2_HOST;
                cfg.spi_mode   = 0;
                cfg.spi_3wire  = false;
                cfg.freq_write = 40000000;
                cfg.freq_read  = 16000000;
                cfg.pin_sclk   = TFT_SCLK;
                cfg.pin_mosi   = TFT_MOSI;
                cfg.pin_miso   = TFT_MISO;
                cfg.pin_dc     = TFT_DC;
                _spi.config(cfg);
            }
            _panel.setBus(&_spi);
            {
                auto cfg       = _panel.config();
                cfg.pin_cs     = TFT_CS;
                cfg.pin_rst    = -1; // reset manuel avant initLCD()
                cfg.offset_rotation = 0;
                _panel.config(cfg);
            }
            setPanel(&_panel);
        }
    };

    static LGFX_ILI9341_Main _ili9341_gpu;
    lgfx::LGFX_Device&      _tft_left  = _ili9341_gpu;
    lgfx::LGFX_Device&      _tft_right = _ili9341_gpu;

#else

    /** Shared SPI Bus configuration pour les yeux GC9A01 */
    class LGFX_SPI_Bus : public lgfx::Bus_SPI {
    public:
        LGFX_SPI_Bus() {
            auto cfg     = config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000;
            cfg.pin_sclk = TFT_SCLK;
            cfg.pin_mosi = TFT_MOSI;
            cfg.pin_miso = TFT_MISO;
            cfg.pin_dc   = TFT_DC;
            config(cfg);
        }
    };

    class LGFX_DualGC9A01 : public lgfx::LGFX_Device {
        lgfx::Panel_GC9A01 _panel_instance;

    public:
        LGFX_DualGC9A01(lgfx::Bus_SPI* shared_bus, int cs_pin) {
            _panel_instance.setBus(shared_bus);

            {
                auto cfg            = _panel_instance.config();
                cfg.pin_cs          = cs_pin;
                cfg.pin_rst         = -1;
                cfg.panel_width     = 240;
                cfg.panel_height    = 240;
                cfg.offset_rotation = 0;
                _panel_instance.config(cfg);
            }
            setPanel(&_panel_instance);
        }
    };

    static LGFX_SPI_Bus    _shared_bus;
    static LGFX_DualGC9A01 _tft_l_dev(&_shared_bus, TFT_CS_L);
    static LGFX_DualGC9A01 _tft_r_dev(&_shared_bus, TFT_CS_R);

    lgfx::LGFX_Device& _tft_left  = _tft_l_dev;
    lgfx::LGFX_Device& _tft_right = _tft_r_dev;

#endif

    LGFX_Sprite _eyeSprites[2][2] = {
        { LGFX_Sprite(&_tft_left), LGFX_Sprite(&_tft_left) },
        { LGFX_Sprite(&_tft_right), LGFX_Sprite(&_tft_right) }
    };
    int         _backBufferIdx = 0;

    bool                    _isInitialized = false;
    Scanline                _circularScanlines[240];
#if defined(SCREEN_ILI9341) && defined(TOUCH_CS)
    static lgfx::Touch_XPT2046 _xpt2046_touch;
    static bool                _ili9341_touch_ready = false;
#endif

    void initLCD(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t led) {
        (void)cs;
        (void)dc;
        (void)rst;

        if (led != 255) {
            pinMode(led, OUTPUT);
            digitalWrite(led, HIGH);
            Serial.printf("LCD: backlight GPIO%u HIGH\n", (unsigned)led);
        } else {
            Serial.println("LCD: WARN TFT_BL indefini → rétroéclairage non piloté");
        }

#if defined(SCREEN_ILI9341)
        if (TFT_RST != -1) {
            pinMode(TFT_CS, OUTPUT);
            digitalWrite(TFT_CS, HIGH);
            pinMode(TFT_RST, OUTPUT);
            digitalWrite(TFT_RST, LOW);
            delay(20);
            digitalWrite(TFT_RST, HIGH);
            delay(120);
        }
#else
        if (TFT_RST != -1) {
            pinMode(TFT_CS_L, OUTPUT); digitalWrite(TFT_CS_L, HIGH);
            pinMode(TFT_CS_R, OUTPUT); digitalWrite(TFT_CS_R, HIGH);

            pinMode(TFT_RST, OUTPUT);
            digitalWrite(TFT_RST, LOW);
            delay(20);
            digitalWrite(TFT_RST, HIGH);
            delay(150);
        }
#endif

#if defined(SCREEN_GC9A01_DUAL)
        for (int y = 0; y < 240; y++) {
            float dy = (float)y - 119.5f;
            float dx = sqrt(max(0.0f, 120.0f * 120.0f - dy * dy));
            _circularScanlines[y].x_start = (int16_t)max(0.0f, 120.0f - dx);
            _circularScanlines[y].x_end   = (int16_t)min(240.0f, 120.0f + dx);
        }

        _tft_left.init();
        _tft_left.setRotation(2);
        _tft_left.invertDisplay(true);
        _tft_left.fillScreen(TFT_BLACK);

        delay(50);

        _tft_right.init();
        _tft_right.setRotation(2);
        _tft_right.invertDisplay(true);
        _tft_right.fillScreen(TFT_BLACK);
#elif defined(SCREEN_ILI9341)
#if defined(TOUCH_CS)
        _ili9341_touch_ready = false;
        {
            auto tc             = _xpt2046_touch.config();
            tc.freq             = SPI_TOUCH_FREQUENCY;
            tc.spi_host         = SPI2_HOST;
            tc.pin_sclk         = TFT_SCLK;
            tc.pin_mosi         = TFT_MOSI;
            tc.pin_miso         = TFT_MISO;
            tc.pin_cs           = TOUCH_CS;
            tc.bus_shared       = true;
            tc.pin_int          = -1;
            /** Lovyan : aligner tactile et `setRotation` (rotation 3 = ±90° inverse parfois mieux selon PCB). */
#if defined(ILI9341_TOUCH_OFFSET_ROT)
            tc.offset_rotation = ILI9341_TOUCH_OFFSET_ROT;
#else
            tc.offset_rotation = 2;
#endif
            _xpt2046_touch.config(tc);
            _ili9341_gpu.panel()->setTouch(&_xpt2046_touch);
        }
#endif
        const bool ili_ok = _ili9341_gpu.init();
        _ili9341_gpu.setRotation(2);
        _ili9341_gpu.invertDisplay(false);
        _ili9341_gpu.fillScreen(TFT_BLACK);
#if defined(TOUCH_CS)
        if (_ili9341_gpu.panel()->getTouch() != nullptr) {
            _ili9341_gpu.panel()->touchCalibrate();
        }
#endif
#if defined(TOUCH_CS)
        _ili9341_touch_ready = ili_ok && (_ili9341_gpu.panel()->getTouch() != nullptr);
        if (!_ili9341_touch_ready) {
            Serial.println(F("WARN: tactile XPT2046 non prêt (init TFT ou touch)."));
        }
#endif
        if (ili_ok) {
            Serial.println(F("LCD: ILI9341 / tactile initialisés (SPI2, TFT_CS + TOUCH_CS)."));
        } else {
            Serial.println(F("LCD: CRITICAL init ILI9341 échoué."));
        }
#else
        _tft_right.init();
        _tft_right.setRotation(2);
        _tft_right.fillScreen(TFT_BLACK);
        _tft_right.invertDisplay(true);
#endif

        bool spritesOk = true;
        for (int eye = 0; eye < 2; eye++) {
            for (int buf = 0; buf < 2; buf++) {
                LGFX_Sprite& spr = _eyeSprites[eye][buf];
                spr.setPsram(true);
                spr.setColorDepth(16);
                spr.createSprite(240, 240);
                if (!spr.getBuffer()) {
                    spr.deleteSprite();
                    spr.setPsram(false);
                    spr.createSprite(240, 240);
                }
                if (!spr.getBuffer()) {
                    Serial.printf("LCD: erreur sprite eye=%d buf=%d\n", eye, buf);
                    spritesOk = false;
                    break;
                }
                spr.setSwapBytes(false);
            }
            if (!spritesOk) break;
        }
        _isInitialized = spritesOk;
        if (!_isInitialized) {
            Serial.println("LCD: CRITICAL sprites 240² non créés → TFT désactivés");
            return;
        }
        clearLCD();

#if !defined(SCREEN_ILI9341)
        _tft_left.fillScreen(TFT_BLACK);
#endif
        _tft_right.fillScreen(TFT_BLACK);
    }

    lgfx::LGFX_Device& getTFT() { return _tft_right; }

    bool tftTouchSubsystemReady() {
#if defined(SCREEN_ILI9341) && defined(TOUCH_CS)
        return _ili9341_touch_ready;
#else
        return false;
#endif
    }

    void clearLCD() {
        if (!_isInitialized) return;
        for (int eye = 0; eye < 2; eye++) {
            for (int buf = 0; buf < 2; buf++) {
                _eyeSprites[eye][buf].fillSprite(TFT_BLACK);
            }
        }
    }
}
