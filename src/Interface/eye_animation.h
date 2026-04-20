#ifndef EYE_ANIMATION_H
#define EYE_ANIMATION_H
#include <Arduino.h>
enum Animation {
  WAKEUP, RESET, MOVE_RIGHT_BIG, MOVE_LEFT_BIG, BLINK_LONG, BLINK_SHORT, HAPPY, SLEEP, SACCADE_RANDOM, CURIOUS, SAD, BORED, LOOKAT, MAX_ANIMATIONS
};
struct EyeState { int height; int width; int x; int y; };
class EyeAnimation {
public:
    EyeAnimation();
    void begin();
    void playAnimation(int animationIndex);
    void resetEyes(bool update = true);
    void blink(int speed = 12);
    void sleep();
    void wakeup();
    void happyEye();
    void saccade(int direction_x, int direction_y);
    void moveRightBigEye();
    void moveLeftBigEye();
    void curious();
    void sad();
    void bored();
    void lookat();
private:
    EyeState left_eye; EyeState right_eye; int corner_radius;
    void drawFrame(); void drawEyes(); void moveBigEye(int direction);
    int calculateSafeRadius(int r, int w, int h);
};
#endif