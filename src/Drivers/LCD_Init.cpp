#include "LCD_Internal.h"

namespace Drivers {
    /** Shared SPI Bus configuration for multi-display environments */
    class LGFX_SPI_Bus : public lgfx::Bus_SPI {
    public:
        LGFX_SPI_Bus() {
            auto cfg = config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000; 
            cfg.pin_sclk = TFT_SCLK;
            cfg.pin_mosi = TFT_MOSI;
            cfg.pin_miso = TFT_MISO;
            cfg.pin_dc   = TFT_DC;
            config(cfg);
        }
    };

    class LGFX_DualGC9A01 : public lgfx::LGFX_Device {
        lgfx::Panel_GC9A01  _panel_instance;

    public:
        LGFX_DualGC9A01(lgfx::Bus_SPI* shared_bus, int cs_pin) {
            _panel_instance.setBus(shared_bus); 

            {
                auto cfg = _panel_instance.config();
                cfg.pin_cs           = cs_pin;
                cfg.pin_rst          = -1; // Reset managed externally
                cfg.panel_width      = 240;
                cfg.panel_height     = 240;
                cfg.offset_rotation  = 0;
                _panel_instance.config(cfg);
            }
            setPanel(&_panel_instance);
        }
    };

    static LGFX_SPI_Bus     _shared_bus;
    static LGFX_DualGC9A01  _tft_l_dev(&_shared_bus, TFT_CS_L);
    static LGFX_DualGC9A01  _tft_r_dev(&_shared_bus, TFT_CS_R);
    
    lgfx::LGFX_Device& _tft_left = _tft_l_dev;
    lgfx::LGFX_Device& _tft_right = _tft_r_dev;

    LGFX_Sprite _eyeSprites[2][2] = {
        { LGFX_Sprite(&_tft_left), LGFX_Sprite(&_tft_left) },
        { LGFX_Sprite(&_tft_right), LGFX_Sprite(&_tft_right) }
    };
    int _backBufferIdx = 0;

    bool _isInitialized = false;
    Scanline _circularScanlines[240];

    void initLCD(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t led) {
        if (led != 255) {
            pinMode(led, OUTPUT);
            digitalWrite(led, HIGH);
        }

        // Synchronized hardware reset for shared RST lines
        if (TFT_RST != -1) {
            pinMode(TFT_CS_L, OUTPUT); digitalWrite(TFT_CS_L, HIGH);
            pinMode(TFT_CS_R, OUTPUT); digitalWrite(TFT_CS_R, HIGH);
            
            pinMode(TFT_RST, OUTPUT);
            digitalWrite(TFT_RST, LOW);
            delay(20);
            digitalWrite(TFT_RST, HIGH);
            delay(150);
        }

#if defined(SCREEN_GC9A01_DUAL)
        // Precompute lookup tables for circular clipping
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
#else
        _tft_right.init();
        _tft_right.setRotation(2);
        _tft_right.fillScreen(TFT_BLACK);
        _tft_right.invertDisplay(true);
#endif

        // Initialisation du Double Buffering en PSRAM
        for (int eye = 0; eye < 2; eye++) {
            for (int buf = 0; buf < 2; buf++) {
                _eyeSprites[eye][buf].setPsram(true); // Force l'usage de la PSRAM
                _eyeSprites[eye][buf].setColorDepth(16);
                _eyeSprites[eye][buf].createSprite(240, 240);
                _eyeSprites[eye][buf].setSwapBytes(false); // Gestion manuelle lors du chargement
            }
        }

        _isInitialized = true;
        clearLCD();
        
        // Finalisation : on s'assure que les écrans sont bien noirs au départ
        _tft_left.fillScreen(TFT_BLACK);
        _tft_right.fillScreen(TFT_BLACK);
    }

    lgfx::LGFX_Device& getTFT() { return _tft_right; }

    void clearLCD() {
        if (!_isInitialized) return;
        for (int eye = 0; eye < 2; eye++) {
            for (int buf = 0; buf < 2; buf++) {
                _eyeSprites[eye][buf].fillSprite(TFT_BLACK);
            }
        }
    }
}