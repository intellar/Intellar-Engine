#include "IMU.h"
#include "Core/EngineState.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

namespace Sensors {

    IMUSensor::IMUSensor(uint8_t address, int index) : _addr(address), _index(index) {}

    bool IMUSensor::begin() {
        // 1. Soft Reset
        Wire.beginTransmission(_addr);
        Wire.write(0x60); Wire.write(0xB0); 
        if (Wire.endTransmission() != 0) return false;
        delay(50); 

        // 2. CTRL1 : Auto-Increment
        Wire.beginTransmission(_addr);
        Wire.write(0x02); 
        Wire.write(0x60); 
        Wire.endTransmission();

        // 3. CTRL2 : Full Scale +/- 2g et ODR 235Hz
        Wire.beginTransmission(_addr);
        Wire.write(0x03); 
        Wire.write(0x05); 
        Wire.endTransmission();

        // 4. CTRL7 : Activer Accel et Gyro
        Wire.beginTransmission(_addr);
        Wire.write(0x08); 
        Wire.write(0x03); 
        Wire.endTransmission();

        _ready = true;
        Serial.printf("Sensors: QMI8658 at 0x%02X initialized (Index %d)\n", _addr, _index);
        return true;
    }

    void IMUSensor::update() {
        if (!_ready) return;

        Wire.beginTransmission(_addr);
        Wire.write(0x35); 
        Wire.endTransmission(false);
        Wire.requestFrom((uint8_t)_addr, (uint8_t)6);

        if (Wire.available() >= 6) {
            // Lecture explicite pour garantir l'ordre des octets (Little Endian)
            uint8_t axL = Wire.read(); uint8_t axH = Wire.read();
            uint8_t ayL = Wire.read(); uint8_t ayH = Wire.read();
            uint8_t azL = Wire.read(); uint8_t azH = Wire.read();

            int16_t raw_ax = (int16_t)(axL | (axH << 8));
            int16_t raw_ay = (int16_t)(ayL | (ayH << 8));
            int16_t raw_az = (int16_t)(azL | (azH << 8));

            float ax_g = raw_ax / 16384.0f;
            float ay_g = raw_ay / 16384.0f;
            float az_g = raw_az / 16384.0f;

            ENGINE_STATE.imuAccel[_index][0] = ax_g;
            ENGINE_STATE.imuAccel[_index][1] = ay_g;
            ENGINE_STATE.imuAccel[_index][2] = az_g;

            float current_roll = atan2(ay_g, az_g);
            float current_pitch = atan2(-ax_g, sqrt(ay_g * ay_g + az_g * az_g));

            // Filtre complémentaire simple
            _lastRoll = _lastRoll * 0.9f + current_roll * 0.1f;
            _lastPitch = _lastPitch * 0.9f + current_pitch * 0.1f;

            ENGINE_STATE.imuRoll[_index].store(_lastRoll);
            ENGINE_STATE.imuPitch[_index].store(_lastPitch);
        }
    }
}