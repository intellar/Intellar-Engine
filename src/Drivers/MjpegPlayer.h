#pragma once

#include <Arduino.h>
#include "LCD.h"

namespace Drivers {
namespace Mjpeg {

bool isPlaying();
bool loopEnabled();
uint8_t speedIndex(); // 0 = 0.5×, 1 = 1×, 2 = 1.5×
float speedMultiplier();

bool play(size_t videoIndex, DisplayIndex target);
void stop();
void toggleLoop();
/** 540 / 541 / 542 */
void setSpeedCommand(int cmd540_542);

/** Appeler à chaque tour de loop() pour cadence et décodage. */
void service(uint32_t nowMs);

} // namespace Mjpeg
} // namespace Drivers
