#include "IskaDemo.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <vector>

#include "IskaLoader.h"
#include "IskaPlayer.h"

namespace Engine {
namespace IskaDemo {

namespace {

    Iska::Asset         s_asset;
    Iska::Player        s_player;
    lgfx::LGFX_Device*  s_panel  = nullptr;
    lgfx::LGFX_Sprite   s_sprite;
    std::vector<uint8_t> s_blob;
    bool                s_ready  = false;

    // The demo stage is portrait (e.g. 240x320) while the ILI9341 panel is
    // landscape (320x240): rotate the frame 90° around its centre so it fills the
    // panel. Flip to -90 (or 270) if it comes out upside down on a given PCB.
    constexpr float    kRotationDeg = 90.0f;
    constexpr uint16_t kBackground  = 0x0000;

    bool loadFirstIska(std::vector<uint8_t>& out) {
        File root = LittleFS.open("/");
        for (File f = root.openNextFile(); f; f = root.openNextFile()) {
            String name = f.name();
            if (!name.endsWith(".iska")) continue;
            size_t n = f.size();
            out.resize(n);
            size_t got = f.read(out.data(), n);
            Serial.printf("[iska] %s: %u B\n", name.c_str(), (unsigned)got);
            return got == n;
        }
        return false;
    }

}  // namespace

bool begin(lgfx::LGFX_Device& panel) {
    s_panel = &panel;

    if (!loadFirstIska(s_blob)) {
        Serial.println("[iska] no .iska on LittleFS "
                       "(pio run -e ili9341_iska -t uploadfs)");
        return false;
    }

    std::string err;
    if (!Iska::parseAsset(s_blob.data(), s_blob.size(), s_asset, &err)) {
        Serial.printf("[iska] parse failed: %s\n", err.c_str());
        return false;
    }
    if (!s_player.bind(&s_asset)) {
        Serial.println("[iska] bind failed");
        return false;
    }
    s_player.playIndex(0, true);

    const int w = s_player.stageWidth();
    const int h = s_player.stageHeight();
    s_sprite.setColorDepth(16);
    s_sprite.setPsram(true);
    s_sprite.createSprite(w, h);
    if (!s_sprite.getBuffer()) {
        s_sprite.deleteSprite();
        s_sprite.setPsram(false);
        s_sprite.createSprite(w, h);
    }
    if (!s_sprite.getBuffer()) {
        Serial.println("[iska] sprite allocation failed");
        return false;
    }

    s_ready = true;
    Serial.printf("[iska] playing '%s' (%dx%d, %u anims)\n",
                  s_player.animationName(), w, h,
                  (unsigned)s_asset.animations.size());
    return true;
}

void update(uint32_t deltaMs) {
    if (s_ready) s_player.update(deltaMs);
}

void render() {
    if (!s_ready || !s_panel) return;

    s_player.render(reinterpret_cast<uint16_t*>(s_sprite.getBuffer()), kBackground);

    // Rotate around the sprite centre, placed at the panel centre.
    s_sprite.setPivot(s_sprite.width() / 2.0f, s_sprite.height() / 2.0f);
    s_panel->startWrite();
    s_sprite.pushRotateZoom(s_panel,
                            s_panel->width() / 2.0f,
                            s_panel->height() / 2.0f,
                            kRotationDeg, 1.0f, 1.0f);
    s_panel->endWrite();
}

}  // namespace IskaDemo
}  // namespace Engine
