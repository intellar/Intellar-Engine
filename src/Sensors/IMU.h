#pragma once
#include <Arduino.h>
#include "Core/MadgwickFilter.h"

namespace Sensors {

/**
 * IMU : Représente un seul capteur QMI8658.
 * Gère la lecture brute I2C et son propre filtre Madgwick d'orientation.
 *
 * Câblage attendu :
 *   ADR = GND → adresse 0x6A (IMU_HEEL)
 *   ADR = VCC → adresse 0x6B (IMU_TOE)
 */
class IMU {
public:
    // beta Madgwick : 0.033 = défaut stable, 0.1 = correction agressive du biais
    explicit IMU(float beta = 0.033f) : _filter(beta) {}

    bool begin(uint8_t addr);
    void update(float dt);
    void reset();

    // Données brutes (g / °/s)
    float ax, ay, az;
    float gx, gy, gz;

    // Données corrigées du biais (°/s)
    float gxc, gyc, gzc;

    // Orientation filtrée (degrés)
    float roll, pitch, yaw;

    bool isReady() const { return _ready; }

    /** Si true : après lecture brute, inverse ax/ay et gx/gy (rotation 180° dans le plan XY) avant biais et Madgwick. */
    void setInvertPlanXY(bool enabled) { _invert_plan_xy = enabled; }

    void setGyroBias(float x, float y, float z) {
        _gx_bias = x; _gy_bias = y; _gz_bias = z;
    }

    /** Quaternion Madgwick courant (unitaire), même convention que getRoll/getPitch/getYaw. */
    void getQuaternion(float& q0, float& q1, float& q2, float& q3) const {
        _filter.getQuaternion(q0, q1, q2, q3);
    }

private:
    uint8_t        _addr  = 0;
    bool           _ready = false;
    MadgwickFilter _filter;

    float _gx_bias = 0.f, _gy_bias = 0.f, _gz_bias = 0.f;
    bool            _invert_plan_xy = false;

    bool _readRaw();
};

} // namespace Sensors
