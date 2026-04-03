#ifndef DRIVERS_OLED_H
#define DRIVERS_OLED_H

namespace Drivers {
    void initOLED();
    void updateOLED(float roll, float pitch, bool btConnected, const float* touchStrengths);
}

#endif