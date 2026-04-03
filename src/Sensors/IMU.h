#ifndef SENSORS_IMU_H
#define SENSORS_IMU_H

namespace Sensors {
    void initIMU();
    bool readIMU(float &roll, float &pitch, float &ax_g, float &ay_g, float &az_g);
}

#endif