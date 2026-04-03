#include "IMU.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

#define QMI_ADDR 0x6B

namespace Sensors {

    static bool imuReady = false;

    void initIMU() {
        // 1. Soft Reset
        Wire.beginTransmission(QMI_ADDR);
        Wire.write(0x60); Wire.write(0xB0); 
        Wire.endTransmission();
        delay(50); 

        // 2. CTRL1 : Auto-Increment
        Wire.beginTransmission(QMI_ADDR);
        Wire.write(0x02); 
        Wire.write(0x60); 
        Wire.endTransmission();

        // 3. CTRL2 : Full Scale +/- 2g et ODR 235Hz
        Wire.beginTransmission(QMI_ADDR);
        Wire.write(0x03); 
        Wire.write(0x05); 
        Wire.endTransmission();

        // 4. CTRL7 : Activer Accel et Gyro
        Wire.beginTransmission(QMI_ADDR);
        Wire.write(0x08); 
        Wire.write(0x03); 
        Wire.endTransmission();

        Serial.println("Sensors: QMI8658 initialise");
        imuReady = true;

        pinMode(17, INPUT); // INT1
        pinMode(16, INPUT); // INT2
    }

    bool readIMU(float &roll, float &pitch, float &ax_g, float &ay_g, float &az_g) {
        if (!imuReady) return false;

        Wire.beginTransmission(QMI_ADDR);
        Wire.write(0x35); 
        Wire.endTransmission(false);
        Wire.requestFrom((uint8_t)QMI_ADDR, (uint8_t)6);

        if (Wire.available() >= 6) {
            uint8_t ax_l = Wire.read();
            uint8_t ax_h = Wire.read();
            uint8_t ay_l = Wire.read();
            uint8_t ay_h = Wire.read();
            uint8_t az_l = Wire.read();
            uint8_t az_h = Wire.read();

            int16_t raw_ax = (int16_t)(ax_l | (ax_h << 8));
            int16_t raw_ay = (int16_t)(ay_l | (ay_h << 8));
            int16_t raw_az = (int16_t)(az_l | (az_h << 8));

            ax_g = raw_ax / 16384.0f;
            ay_g = raw_ay / 16384.0f;
            az_g = raw_az / 16384.0f;

            float current_roll = atan2(ay_g, az_g);
            float current_pitch = atan2(-ax_g, sqrt(ay_g * ay_g + az_g * az_g));

            if (roll == 0 && pitch == 0) {
                roll = current_roll;
                pitch = current_pitch;
            } else {
                roll = roll * 0.9f + current_roll * 0.1f;
                pitch = pitch * 0.9f + current_pitch * 0.1f;
            }
            return true;
        }
        return false;
    }
}