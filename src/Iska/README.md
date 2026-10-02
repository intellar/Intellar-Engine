# ISKA in Intellar-Engine

ISKA is a reusable, host-agnostic **cut-out bone animation** runtime (skeleton +
keyframes + RGB565 affine blit). It is copied **verbatim** into `lib/Iska/` from the
`Intellar-Engine-Animation` repository (`src/Iska/`), where the format, the tooling
and the `.iska` asset pipeline live.

- **`lib/Iska/`** — the library (MIT). No Arduino, no SDL, no filesystem: the host
  owns the framebuffer, the clock and the panel. The animation repo's
  `tools/iska_generic_check.py` enforces that it stays host- and character-agnostic.
- **`src/Iska/IskaDemo.*`** — demo glue that loads the first `*.iska` from LittleFS
  and renders it to the panel. It never names a specific asset or character.

## Build the demo

`[env:ili9341_iska]` in `platformio.ini` is the ISKA demo target (it just adds
`-DHAS_ISKA` to the normal `ili9341` target):

```sh
pio run -e ili9341_iska -t uploadfs    # flash data/ (the .iska asset) to LittleFS
pio run -e ili9341_iska -t upload      # flash the firmware
```

## Adding your own animation

Drop any `.iska` file into `data/` and re-run `uploadfs`. The demo plays the first
`.iska` it finds (alphabetical). The asset is **data**, not code: the engine source
never mentions a specific character.

To keep `lib/Iska/` in sync with the animation repo:

```sh
cp ../Intellar-Engine-Animation/src/Iska/* lib/Iska/
```
