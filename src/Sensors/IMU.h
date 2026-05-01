#pragma once
#include <Arduino.h>
#include "Core/MadgwickFilter.h"

namespace Sensors {

    /**
     * IMU : Représente un seul capteur QMI8658.
     * Gère la lecture brute et son propre filtre d'orientation.
     */
    class IMU {
    public:
        explicit IMU(float beta = 0.033f) : _filter(beta) {}

        bool begin(uint8_t addr);
        void update(float dt);

        // Données brutes
        float ax, ay, az; // Accélération (g)
        float gx, gy, gz; // Rotation (°/s)

        // Orientation filtrée
        float roll, pitch, yaw;

        bool isReady() const { return _ready; }

        // Définit les offsets à soustraire aux lectures brutes (°/s)
        void setGyroBias(float x, float y, float z) {
            _gx_bias = x; _gy_bias = y; _gz_bias = z;
        }

        // Valeurs corrigées (après soustraction du biais)
        float gxc, gyc, gzc;

        void reset(); // Remet le filtre d'orientation à zéro

    private:
        uint8_t        _addr = 0;
        bool           _ready = false;
        MadgwickFilter _filter;

        float _gx_bias = 0.f, _gy_bias = 0.f, _gz_bias = 0.f;

        bool _readRaw();
    };

} // namespace Sensors
