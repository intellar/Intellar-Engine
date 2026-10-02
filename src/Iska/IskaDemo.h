#pragma once
#include <cstdint>
#include <LovyanGFX.hpp>

// ISKA demo glue: plays the first `*.iska` found on LittleFS onto a panel.
// Only compiled when HAS_ISKA is defined (see platformio.ini: [env:ili9341_iska]).
// Character-agnostic: it never names any specific asset or character.
namespace Engine {
namespace IskaDemo {

    // Loads the first `*.iska` on LittleFS, parses it and binds the player.
    // Returns false (with a Serial log) on any error.
    bool begin(lgfx::LGFX_Device& panel);

    // Advances the animation clock by `deltaMs`.
    void update(uint32_t deltaMs);

    // Renders the current pose onto the bound panel.
    void render();

}  // namespace IskaDemo
}  // namespace Engine
