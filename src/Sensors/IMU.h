#ifndef SENSORS_IMU_H
#define SENSORS_IMU_H
#include <Arduino.h>

namespace Sensors {
    class IMUSensor {
    public:
        IMUSensor(uint8_t address, int index);
        bool begin();
        void update();
        bool isAlive() const { return _ready; }

    private:
        uint8_t _addr;
        int _index;
        bool _ready = false;
        float _lastRoll = 0.0f;
        float _lastPitch = 0.0f;
    };
}
#endif