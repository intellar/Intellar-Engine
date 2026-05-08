#include "IMU.h"
#include <Wire.h>

namespace Sensors {

// ─── Registres QMI8658 ────────────────────────────────────────────────────────
static constexpr uint8_t REG_RESET = 0x60;
static constexpr uint8_t REG_CTRL1 = 0x02;
static constexpr uint8_t REG_CTRL2 = 0x03;
static constexpr uint8_t REG_CTRL3 = 0x04;
static constexpr uint8_t REG_CTRL7 = 0x08;

// Facteurs de conversion
// CTRL2 = 0x05 → ±2g    → 16384 LSB/g
// CTRL3 = 0x54 → ±512dps → 64 LSB/dps
static constexpr float ACCEL_SCALE = 1.f / 16384.f;
static constexpr float GYRO_SCALE  = 1.f / 64.f;
// ─────────────────────────────────────────────────────────────────────────────

bool IMU::begin(uint8_t addr) {
    _addr = addr;

    // Soft reset
    Wire.beginTransmission(_addr);
    Wire.write(REG_RESET); Wire.write(0xB0);
    if (Wire.endTransmission() != 0) return false;
    delay(100);

    // CTRL1 : auto-increment activé, little-endian
    Wire.beginTransmission(_addr);
    Wire.write(REG_CTRL1); Wire.write(0x40);
    Wire.endTransmission();

    // CTRL2 : accel ±2g, ODR 235 Hz
    Wire.beginTransmission(_addr);
    Wire.write(REG_CTRL2); Wire.write(0x05);
    Wire.endTransmission();

    // CTRL3 : gyro ±512 dps, ODR 235 Hz
    Wire.beginTransmission(_addr);
    Wire.write(REG_CTRL3); Wire.write(0x54);
    Wire.endTransmission();

    // CTRL7 : active accel + gyro
    Wire.beginTransmission(_addr);
    Wire.write(REG_CTRL7); Wire.write(0x03);
    Wire.endTransmission();

    delay(50); // Attendre stabilisation ODR

    // Purge des premiers samples invalides
    for (int i = 0; i < 30; i++) { _readRaw(); delay(5); }

    // Calibration du biais gyro — robuste aux mouvements au boot.
    // Si un biais estimé est irréaliste, on retente plutôt que de valider une mauvaise base.
    Serial.printf("[IMU] Calibration biais gyro @ 0x%02X...\n", addr);
    constexpr int   kGyroCalSamples   = 64;
    constexpr int   kGyroCalAttempts  = 4;
    constexpr float kMaxAcceptedBiasDps = 5.0f;
    constexpr int   kMinValidSamples  = 10;

    bool  biasAccepted = false;
    int   lastValidCount = 0;
    float lastBx = 0.f, lastBy = 0.f, lastBz = 0.f;

    for (int attempt = 1; attempt <= kGyroCalAttempts; ++attempt) {
        float sx = 0.f, sy = 0.f, sz = 0.f;
        int   validCount = 0;

        for (int i = 0; i < kGyroCalSamples; i++) {
            if (_readRaw()) {
                float gxCal = gx;
                float gyCal = gy;
                float gzCal = gz;

                // Calibrer le biais gyro dans le même repère que le runtime update().
                if (_invert_plan_xy) {
                    gxCal = -gxCal;
                    gyCal = -gyCal;
                }

                if (fabsf(gxCal) < 300.f && fabsf(gyCal) < 300.f && fabsf(gzCal) < 300.f) {
                    sx += gxCal;
                    sy += gyCal;
                    sz += gzCal;
                    validCount++;
                }
            }
            delay(5);
        }

        if (validCount < kMinValidSamples) {
            Serial.printf("[IMU] Tentative %d/%d rejetee: %d samples valides (min %d)\n",
                attempt, kGyroCalAttempts, validCount, kMinValidSamples);
            delay(120);
            continue;
        }

        const float bx = sx / validCount;
        const float by = sy / validCount;
        const float bz = sz / validCount;
        const float maxAbsBias = max(max(fabsf(bx), fabsf(by)), fabsf(bz));

        lastValidCount = validCount;
        lastBx = bx;
        lastBy = by;
        lastBz = bz;

        if (maxAbsBias <= kMaxAcceptedBiasDps) {
            _gx_bias = bx;
            _gy_bias = by;
            _gz_bias = bz;
            biasAccepted = true;
            break;
        }

        Serial.printf("[IMU] Tentative %d/%d rejetee: biais trop grand (%.3f / %.3f / %.3f °/s)\n",
            attempt, kGyroCalAttempts, bx, by, bz);
        delay(120);
    }

    if (!biasAccepted) {
        if (lastValidCount < kMinValidSamples) {
            Serial.printf("[IMU] ERREUR: calibration gyro impossible @ 0x%02X\n", addr);
            return false;
        }

        // Compatibilité comportement runtime: conserver la meilleure estimation disponible,
        // même si elle est élevée, pour éviter un drift encore pire avec biais forcé à 0.
        _gx_bias = lastBx;
        _gy_bias = lastBy;
        _gz_bias = lastBz;
        Serial.printf("[IMU] ATTENTION: biais eleve conserve (%.3f / %.3f / %.3f °/s) apres %d tentatives\n",
            lastBx, lastBy, lastBz, kGyroCalAttempts);
    }

    Serial.printf("[IMU] Biais gyro: %.3f / %.3f / %.3f °/s\n", _gx_bias, _gy_bias, _gz_bias);

    _ready = true;
    Serial.printf("[IMU] QMI8658 @ 0x%02X prêt\n", addr);
    return true;
}

bool IMU::_readRaw() {
    // Rafale 12 octets depuis AX_L (0x35) : accel puis gyro, même instant d’échantillonnage
    // (auto-incrément CTRL1) — évite deux transactions I2C et désalignement accel/gyro.
    Wire.beginTransmission(_addr);
    Wire.write(0x35);
    if (Wire.endTransmission(false) != 0) return false;
    const uint8_t n = Wire.requestFrom(_addr, (uint8_t)12);
    if (n != 12) {
        while (Wire.available()) (void)Wire.read();
        return false;
    }
    uint8_t buf[12];
    for (int i = 0; i < 12; i++) buf[i] = Wire.read();

    int16_t raw_ax = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    int16_t raw_ay = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
    int16_t raw_az = (int16_t)((uint16_t)buf[4] | ((uint16_t)buf[5] << 8));
    int16_t raw_gx = (int16_t)((uint16_t)buf[6] | ((uint16_t)buf[7] << 8));
    int16_t raw_gy = (int16_t)((uint16_t)buf[8] | ((uint16_t)buf[9] << 8));
    int16_t raw_gz = (int16_t)((uint16_t)buf[10] | ((uint16_t)buf[11] << 8));

    ax = raw_ax * ACCEL_SCALE;
    ay = raw_ay * ACCEL_SCALE;
    az = raw_az * ACCEL_SCALE;
    gx = raw_gx * GYRO_SCALE;
    gy = raw_gy * GYRO_SCALE;
    gz = raw_gz * GYRO_SCALE;

    return true;
}

void IMU::reset() {
    _filter.reset();  // conserve le beta du constructeur IMU
}

void IMU::update(float dt) {
    if (!_ready || !_readRaw()) return;

    if (_invert_plan_xy) {
        ax = -ax;
        ay = -ay;
        gx = -gx;
        gy = -gy;
    }

    gxc = gx - _gx_bias;
    gyc = gy - _gy_bias;
    gzc = gz - _gz_bias;

    _filter.update(gxc, gyc, gzc, ax, ay, az, dt);
    roll  = _filter.getRoll();
    pitch = _filter.getPitch();
    yaw   = _filter.getYaw();
}

} // namespace Sensors
