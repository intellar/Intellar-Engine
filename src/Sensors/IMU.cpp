#include "IMU.h"
#include <Wire.h>

namespace Sensors {

// ─── Registres QMI8658 ────────────────────────────────────────────────────────
static constexpr uint8_t REG_RESET  = 0x60;
static constexpr uint8_t REG_CTRL1  = 0x02;
static constexpr uint8_t REG_CTRL2  = 0x03;
static constexpr uint8_t REG_CTRL3  = 0x04;
static constexpr uint8_t REG_CTRL7  = 0x08;
static constexpr uint8_t REG_DATA   = 0x35; // Premier registre de données accel

// Facteurs de conversion — doivent correspondre à la config CTRL2/CTRL3
// CTRL2 = 0x05 → ±2g   → 16384 LSB/g
// CTRL3 = 0x04 → ±512dps → 64 LSB/dps
static constexpr float ACCEL_SCALE = 1.f / 16384.f;
static constexpr float GYRO_SCALE  = 1.f / 64.f;
// ─────────────────────────────────────────────────────────────────────────────

bool IMU::begin(uint8_t addr) {
    _addr = addr;
    
    
    
    // Soft reset
    Wire.beginTransmission(_addr);
    Wire.write(REG_RESET); Wire.write(0xB0);
    if (Wire.endTransmission() != 0) return false;
    delay(50);

    // CTRL1 : auto-increment activé
    Wire.beginTransmission(_addr);
    Wire.write(REG_CTRL1); Wire.write(0x60);
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

    _ready = true;
    Serial.printf("[IMU] QMI8658 @ 0x%02X Initialisé\n", addr);


    Wire.beginTransmission(_addr);
    Wire.write(REG_CTRL3);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)1);
    Serial.printf("[CTRL3 readback 0x%02X] = 0x%02X\n", _addr, Wire.read());


    Wire.beginTransmission(_addr);
    Wire.write(0x00);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)1);
    uint8_t whoami = Wire.read();
    Serial.printf("[IMU] WHO_AM_I @ 0x%02X = 0x%02X %s\n", 
        addr, whoami, whoami == 0x05 ? "OK" : "ERREUR");
    
    

    // Calibration du biais gyro : moyenne sur 200 samples à l'arrêt
    Serial.printf("[IMU] Calibration biais gyro @ 0x%02X...\n", addr);
    float sx = 0, sy = 0, sz = 0;
    const int N = 50;
    for (int i = 0; i < N; i++) {
        if (_readRaw()) { sx += gx; sy += gy; sz += gz; }
        delay(5);
    }
    _gx_bias = sx / N;
    _gy_bias = sy / N;
    _gz_bias = sz / N;
    Serial.printf("[IMU] Biais gyro: %.3f / %.3f / %.3f °/s\n",
                  _gx_bias, _gy_bias, _gz_bias);

    Serial.printf("[IMU] QMI8658 @ 0x%02X Initialisé\n", addr);
    return true;
}
    

bool IMU::_readRaw() {
    // Lire accel (0x35)
    Wire.beginTransmission(_addr);
    Wire.write(0x35);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)6);
    uint8_t abuf[6];
    for (int i = 0; i < 6; i++) abuf[i] = Wire.read();

    // Lire gyro (0x3B)
    Wire.beginTransmission(_addr);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)6);
    uint8_t gbuf[6];
    for (int i = 0; i < 6; i++) gbuf[i] = Wire.read();

    int16_t raw_ax = (int16_t)((uint16_t)abuf[0] | ((uint16_t)abuf[1] << 8));
    int16_t raw_ay = (int16_t)((uint16_t)abuf[2] | ((uint16_t)abuf[3] << 8));
    int16_t raw_az = (int16_t)((uint16_t)abuf[4] | ((uint16_t)abuf[5] << 8));
    int16_t raw_gx = (int16_t)((uint16_t)gbuf[0] | ((uint16_t)gbuf[1] << 8));
    int16_t raw_gy = (int16_t)((uint16_t)gbuf[2] | ((uint16_t)gbuf[3] << 8));
    int16_t raw_gz = (int16_t)((uint16_t)gbuf[4] | ((uint16_t)gbuf[5] << 8));

    ax = raw_ax * ACCEL_SCALE;
    ay = raw_ay * ACCEL_SCALE;
    az = raw_az * ACCEL_SCALE;
    gx = raw_gx * GYRO_SCALE;
    gy = raw_gy * GYRO_SCALE;
    gz = raw_gz * GYRO_SCALE;

    return true;
}

void IMU::reset() {
    _filter = MadgwickFilter(); // Réinitialise l'état interne du filtre
}

void IMU::update(float dt) {
    if (!_ready || !_readRaw()) return;

    // Soustraction du biais avant de donner les valeurs au filtre
    gxc = gx - _gx_bias;
    gyc = gy - _gy_bias;
    gzc = gz - _gz_bias;

    _filter.update(gxc, gyc, gzc, ax, ay, az, dt);
    roll  = _filter.getRoll();
    pitch = _filter.getPitch();
    yaw   = _filter.getYaw();
}

} // namespace Sensors
