#include "MjpegPlayer.h"
#include "Core/EngineState.h"
#include <LittleFS.h>
#include <JPEGDEC.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Drivers {
namespace Mjpeg {

namespace {

static constexpr size_t kJpegStagingMax = 400 * 1024;
static constexpr int kCanvas        = 240;
static constexpr int kNominalFps    = 20;
/** Limite JPEG (px) pour buffer raster décodé (~PSRAM). */
static constexpr int kJpegDecodeMaxSide = 512;

File      g_file{};
uint8_t*  g_jpegBuf    = nullptr;
uint16_t* g_rgb565     = nullptr;
uint16_t* g_natRgb     = nullptr;
size_t    g_natCapElts = 0;
/** Stride lignes (= largeur raster décodée) pendant le JPEG_DRAW. */
int       g_natStride  = 0;
int       g_natHeight  = 0;

bool      g_playing    = false;
bool      g_loop       = true;
uint8_t   g_speedIdx   = 1; // 1×
DisplayIndex g_side    = DisplayIndex::RIGHT;
unsigned long g_lastFrameMs = 0;
/** g_speedIdx==2 : apres decode, tous les autres pas on saute aussi un JPEG (lecture sans decode). */
uint32_t  g_skip151Acc = 0;

JPEGDEC   g_dec;
uint16_t* g_drawTarget = nullptr;

int jpegDraw(JPEGDRAW* p) {
    if (!g_drawTarget || !p->pPixels || g_natStride <= 0 || g_natHeight <= 0) return 0;
    const int x0 = p->x;
    const int y0 = p->y;
    const int w  = p->iWidthUsed > 0 ? p->iWidthUsed : p->iWidth;
    const int h  = p->iHeight;
    const int sw = p->iWidth;

    uint16_t* src = p->pPixels;

    for (int row = 0; row < h; row++) {
        const int y = y0 + row;
        if (y < 0 || y >= g_natHeight) continue;

        const int rx0 = x0;
        const int rx1 = x0 + w;
        const int dstL = std::max(0, rx0);
        const int dstR = std::min(g_natStride, rx1);
        const int cw   = dstR - dstL;
        if (cw <= 0) continue;

        const int srcCol = dstL - x0;
        uint16_t* dst    = g_drawTarget + static_cast<size_t>(y) * static_cast<size_t>(g_natStride)
                        + static_cast<size_t>(dstL);
        memcpy(dst, src + static_cast<size_t>(row) * static_cast<size_t>(sw)
                        + static_cast<size_t>(srcCol),
               static_cast<size_t>(cw) * sizeof(uint16_t));
    }
    return 1;
}

/** Réduit ow×oh (stride srcStride ≥ ow) dans 240², proportions préservées ; bandes noires si cadre ≠ carré. */
static void scaleContain240(const uint16_t* src, int ow, int oh, int srcStride, uint16_t* dst) {
    memset(dst, 0, static_cast<size_t>(kCanvas) * static_cast<size_t>(kCanvas) * sizeof(uint16_t));
    if (!src || ow <= 0 || oh <= 0 || srcStride < ow || ow > kJpegDecodeMaxSide || oh > kJpegDecodeMaxSide)
        return;

    const float sc =
        std::min((float)(kCanvas - 1) / (float)ow, (float)(kCanvas - 1) / (float)oh);
    const int nw = std::max(1, (int)std::lround((double)ow * (double)sc));
    const int nh = std::max(1, (int)std::lround((double)oh * (double)sc));
    const int offx = (kCanvas - nw) / 2;
    const int offy = (kCanvas - nh) / 2;

    for (int dy = 0; dy < nh; dy++) {
        const int sy = std::min(oh - 1, (int)(((int64_t)dy * oh + nh / 2) / nh));
        for (int dx = 0; dx < nw; dx++) {
            const int sx = std::min(ow - 1, (int)(((int64_t)dx * ow + nw / 2) / nw));
            dst[static_cast<size_t>(offy + dy) * kCanvas + static_cast<size_t>(offx + dx)] =
                src[static_cast<size_t>(sy) * static_cast<size_t>(srcStride) + static_cast<size_t>(sx)];
        }
    }
}

static bool ensureNatRaster(size_t minElts) {
    if (minElts == 0 || minElts > (size_t)kJpegDecodeMaxSide * (size_t)kJpegDecodeMaxSide)
        return false;
    if (minElts <= g_natCapElts && g_natRgb) return true;
    if (g_natRgb) {
        free(g_natRgb);
        g_natRgb = nullptr;
        g_natCapElts = 0;
    }
    g_natRgb = (uint16_t*)ps_malloc(minElts * sizeof(uint16_t));
    if (!g_natRgb) return false;
    g_natCapElts = minElts;
    return true;
}

bool ensureBuffers() {
    if (!g_jpegBuf) {
        g_jpegBuf = (uint8_t*)ps_malloc(kJpegStagingMax);
        if (!g_jpegBuf) return false;
    }
    if (!g_rgb565) {
        g_rgb565 = (uint16_t*)ps_malloc(kCanvas * kCanvas * sizeof(uint16_t));
        if (!g_rgb565) return false;
    }
    return true;
}

/** Lit un JPEG du flux (SOI … EOI) depuis la position courante du fichier. */
size_t readOneJpegFromFile(File& f, uint8_t* dst, size_t cap) {
    int c;
    while (true) {
        c = f.read();
        if (c < 0) return 0;
        if (c != 0xFF) continue;
        c = f.read();
        if (c < 0) return 0;
        if (c == 0xD8) {
            if (2 > cap) return 0;
            dst[0]      = 0xFF;
            dst[1]      = 0xD8;
            size_t len  = 2;
            int    prev = 0xD8;
            while (len < cap) {
                c = f.read();
                if (c < 0) return 0;
                dst[len++] = (uint8_t)c;
                if (prev == 0xFF && c == 0xD9) return len;
                prev = c;
            }
            return 0;
        }
    }
}

bool decodeJpegToRgb565(const uint8_t* data, size_t len) {
    if (!g_rgb565 || len < 4) return false;
    memset(g_rgb565, 0, static_cast<size_t>(kCanvas) * static_cast<size_t>(kCanvas) * sizeof(uint16_t));

    if (g_dec.openRAM(const_cast<uint8_t*>(data), (int)len, jpegDraw) != 1)
        return false;

    /** Après decode + JPEG_AUTO_ROTATE, getWidth/Height correspondent au raster livré dans jpegDraw. */
    const int hdrW = std::max(1, g_dec.getWidth());
    const int hdrH = std::max(1, g_dec.getHeight());
    const int capW = hdrW > hdrH ? hdrW : hdrH;
    const int capH = hdrW > hdrH ? hdrW : hdrH;
    const size_t needElts = static_cast<size_t>(capW) * static_cast<size_t>(capH);
    if (capW > kJpegDecodeMaxSide || capH > kJpegDecodeMaxSide || !ensureNatRaster(needElts)) {
        g_drawTarget = nullptr;
        g_natStride  = 0;
        g_natHeight  = 0;
        g_dec.close();
        Serial.printf("Mjpeg: JPEG trop grand (%dx%d header) ou memoire native\n", hdrW, hdrH);
        return false;
    }

    g_natStride = capW;
    g_natHeight = capH;
    memset(g_natRgb, 0, needElts * sizeof(uint16_t));
    g_drawTarget = g_natRgb;
    g_dec.setPixelType(RGB565_LITTLE_ENDIAN);

    const int ok           = g_dec.decode(0, 0, JPEG_AUTO_ROTATE);
    const int ow           = std::max(1, g_dec.getWidth());
    const int oh           = std::max(1, g_dec.getHeight());
    const int rasterStride = g_natStride;
    g_dec.close();

    g_drawTarget = nullptr;
    g_natStride    = 0;
    g_natHeight    = 0;

    if (ok != 1) return false;

    if (ow == kCanvas && oh == kCanvas && rasterStride >= kCanvas) {
        if (rasterStride == kCanvas) {
            memcpy(g_rgb565, g_natRgb,
                   static_cast<size_t>(kCanvas) * static_cast<size_t>(kCanvas) * sizeof(uint16_t));
        } else {
            for (int y = 0; y < kCanvas; y++) {
                memcpy(g_rgb565 + static_cast<size_t>(y) * static_cast<size_t>(kCanvas),
                       g_natRgb + static_cast<size_t>(y) * static_cast<size_t>(rasterStride),
                       static_cast<size_t>(kCanvas) * sizeof(uint16_t));
            }
        }
        return true;
    }
    scaleContain240(g_natRgb, ow, oh, rasterStride, g_rgb565);
    static uint32_t dbgCount = 0;
    if (++dbgCount <= 8u)
        Serial.printf("Mjpeg: frame %dx%d -> 240² contain\n", ow, oh);
    return true;
}

/** Délais entre deux images affichées (le decode peut prendre plus : on ne force pas plusieurs decodes/frame). */
static uint32_t displayHoldMs() {
    const uint32_t base = (uint32_t)((1000.f / (float)kNominalFps) + 0.5f);
    if (g_speedIdx == 0)
        return base * 2u;
    return base;
}

/** Lit un bloc JPEG suivant depuis le fichier sans decode (positions après EOI). */
static bool advancePastOneJpeg(File& f) {
    uint8_t* buf = g_jpegBuf;
    size_t   n   = readOneJpegFromFile(f, buf, kJpegStagingMax);
    if (n != 0) return true;
    if (g_loop && f) {
        f.seek(0);
        n = readOneJpegFromFile(f, buf, kJpegStagingMax);
    }
    return n != 0;
}

void closeFile() {
    if (g_file) {
        g_file.close();
    }
}

} // namespace

bool isPlaying() { return g_playing; }

bool loopEnabled() { return g_loop; }

uint8_t speedIndex() { return g_speedIdx; }

float speedMultiplier() {
    if (g_speedIdx == 0) return 0.5f;
    if (g_speedIdx == 1) return 1.f;
    return 1.5f;
}

void setSpeedCommand(int cmd) {
    if (cmd == 540) g_speedIdx = 0;
    else if (cmd == 541) g_speedIdx = 1;
    else if (cmd == 542) g_speedIdx = 2;
    else return;
    g_skip151Acc = 0;
}

void releaseNatRaster() {
    if (g_natRgb) {
        free(g_natRgb);
        g_natRgb     = nullptr;
        g_natCapElts = 0;
    }
}

bool play(size_t videoIndex, DisplayIndex target) {
    if (videoIndex >= ENGINE_STATE.videoFiles.size()) {
        Serial.println("Mjpeg::play index hors plage");
        return false;
    }
    if (!ensureBuffers()) {
        Serial.println("Mjpeg::play memoire insuffisante");
        return false;
    }
    stop();
    const char* path = ENGINE_STATE.videoFiles[videoIndex].c_str();
    g_file           = LittleFS.open(path, "r");
    if (!g_file) {
        Serial.printf("Mjpeg::play open fail %s\n", path);
        return false;
    }
    g_side          = target;
    g_playing       = true;
    g_lastFrameMs   = 0;
    g_skip151Acc    = 0;
    Serial.printf("Mjpeg::play %s\n", path);
    return true;
}

void stop() {
    closeFile();
    g_playing = false;
    g_lastFrameMs = 0;
    g_skip151Acc = 0;
    releaseNatRaster();
}

void toggleLoop() {
    g_loop = !g_loop;
}

void service(uint32_t nowMs) {
    if (!g_playing || !g_file) return;
    const uint32_t interval = displayHoldMs();
    if (g_lastFrameMs != 0 && (uint32_t)(nowMs - g_lastFrameMs) < interval) return;

    size_t n = readOneJpegFromFile(g_file, g_jpegBuf, kJpegStagingMax);
    if (n == 0) {
        if (g_loop && g_file) {
            g_file.seek(0);
            n = readOneJpegFromFile(g_file, g_jpegBuf, kJpegStagingMax);
        }
        if (n == 0) {
            Serial.println("Mjpeg: fin / erreur flux");
            stop();
            return;
        }
    }

    if (!decodeJpegToRgb565(g_jpegBuf, n)) {
        Serial.println("Mjpeg: decode JPEG echoue, frame saute");
        g_lastFrameMs = nowMs;
        return;
    }

    pushVideo565(g_side, g_rgb565);
    g_lastFrameMs = nowMs;

    if (g_speedIdx == 2) {
        g_skip151Acc++;
        if (g_skip151Acc & 1u)
            (void)advancePastOneJpeg(g_file);
    }
}

} // namespace Mjpeg
} // namespace Drivers
