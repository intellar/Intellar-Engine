#include "eye_animation.h"
#include "display_wrapper.h"

static const int REF_EYE_HEIGHT = 40;
static const int REF_EYE_WIDTH = 40;
static const int REF_SPACE_BETWEEN_EYE = 10;
static const int REF_CORNER_RADIUS = 10;

EyeAnimation::EyeAnimation() {
    corner_radius = REF_CORNER_RADIUS;
}

void EyeAnimation::begin() {
    g_init_display();
    resetEyes(false);
    drawFrame(); 
}

int EyeAnimation::calculateSafeRadius(int r, int w, int h) {
    if (w < 2 * (r + 1)) r = (w / 2) - 1;
    if (h < 2 * (r + 1)) r = (h / 2) - 1;
    return (r < 0) ? 0 : r;
}

void EyeAnimation::drawEyes() {
    int r_left = calculateSafeRadius(corner_radius, left_eye.width, left_eye.height);
    int x_left = int(left_eye.x - left_eye.width / 2);
    int y_left = int(left_eye.y - left_eye.height / 2);
    g_draw_filled_round_rect(x_left, y_left, left_eye.width, left_eye.height, r_left, G_COLOR_WHITE);

    int r_right = calculateSafeRadius(corner_radius, right_eye.width, right_eye.height);
    int x_right = int(right_eye.x - right_eye.width / 2);
    int y_right = int(right_eye.y - right_eye.height / 2);
    g_draw_filled_round_rect(x_right, y_right, right_eye.width, right_eye.height, r_right, G_COLOR_WHITE);
}

void EyeAnimation::drawFrame() {
    g_clear_display();
    drawEyes();
    g_update_display();
}

void EyeAnimation::resetEyes(bool update) {
    left_eye.height = REF_EYE_HEIGHT;
    left_eye.width = REF_EYE_WIDTH;
    right_eye.height = REF_EYE_HEIGHT;
    right_eye.width = REF_EYE_WIDTH;
    left_eye.x = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
    left_eye.y = SCREEN_HEIGHT / 2;
    right_eye.x = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;
    right_eye.y = SCREEN_HEIGHT / 2;
    corner_radius = REF_CORNER_RADIUS;
    if (update) drawFrame();
}

void EyeAnimation::blink(int speed) {
    resetEyes(false);
    for(int i=0; i<3; i++) {
        left_eye.height -= speed; right_eye.height -= speed;
        int current_h = left_eye.height;
        int mapped_radius = map(current_h, 4, REF_EYE_HEIGHT, 1, REF_CORNER_RADIUS);
        corner_radius = min(mapped_radius, current_h / 2);
        left_eye.width += 3; right_eye.width += 3;
        drawFrame(); delay(1);
    }
    for(int i=0; i<3; i++) {
        left_eye.height += speed; right_eye.height += speed;
        int current_h = left_eye.height;
        int mapped_radius = map(current_h, 4, REF_EYE_HEIGHT, 1, REF_CORNER_RADIUS);
        corner_radius = min(mapped_radius, current_h / 2);
        left_eye.width -= 3; right_eye.width -= 3;
        drawFrame(); delay(1);
    }
    resetEyes();
}

void EyeAnimation::sleep() {
    resetEyes(false);
    left_eye.height = 2; left_eye.width = REF_EYE_WIDTH;
    right_eye.height = 2; right_eye.width = REF_EYE_WIDTH;
    corner_radius = 0;
    drawFrame();
}

void EyeAnimation::wakeup() {
    resetEyes(false);
    for(int h = 2; h <= REF_EYE_HEIGHT; h += 2) {
        left_eye.height = h; right_eye.height = h;    
        int mapped_radius = map(h, 2, REF_EYE_HEIGHT, 1, REF_CORNER_RADIUS);
        corner_radius = min(mapped_radius, h / 2);
        drawFrame();
    }
}

void EyeAnimation::happyEye() {
    resetEyes(true); 
    int offset = REF_EYE_HEIGHT / 2;
    for(int i=0; i<10; i++) {
        g_draw_filled_triangle(
            left_eye.x - left_eye.width/2-1, left_eye.y+offset, 
            left_eye.x + left_eye.width/2+1, left_eye.y+5+offset, 
            left_eye.x - left_eye.width/2-1, left_eye.y+left_eye.height+offset, 
            G_COLOR_BLACK);    
        g_draw_filled_triangle(
            right_eye.x + right_eye.width/2+1, right_eye.y+offset, 
            right_eye.x - right_eye.width/2-2, right_eye.y+5+offset, 
            right_eye.x + right_eye.width/2+1, right_eye.y+right_eye.height+offset, 
            G_COLOR_BLACK);
        offset -= 2;
        g_update_display(); delay(1);
    }
    delay(1000);
    resetEyes();
}

void EyeAnimation::saccade(int direction_x, int direction_y) {
    const int MOVEMENT_AMPLITUDE_X = 8;
    const int MOVEMENT_AMPLITUDE_Y = 6;
    const int BLINK_AMPLITUDE = 8;
    for(int i = 1; i <= 2; i++) {
        left_eye.x += MOVEMENT_AMPLITUDE_X * direction_x;
        right_eye.x += MOVEMENT_AMPLITUDE_X * direction_x;
        left_eye.y += MOVEMENT_AMPLITUDE_Y * direction_y;
        right_eye.y += MOVEMENT_AMPLITUDE_Y * direction_y;
        int height_change = (i == 1) ? -BLINK_AMPLITUDE : BLINK_AMPLITUDE;
        right_eye.height += height_change;
        left_eye.height += height_change;
        drawFrame(); delay(1);
    }
}

void EyeAnimation::moveBigEye(int direction) {
    resetEyes(false);
    const int OVERSIZE_AMOUNT = 1;
    const int MOVEMENT_AMPLITUDE = 2;
    const int BLINK_AMPLITUDE = 5;
    for(int i=0; i<3; i++) {
        left_eye.x += MOVEMENT_AMPLITUDE * direction;
        right_eye.x += MOVEMENT_AMPLITUDE * direction;    
        right_eye.height -= BLINK_AMPLITUDE;
        left_eye.height -= BLINK_AMPLITUDE;
        EyeState* t = (direction > 0) ? &right_eye : &left_eye;
        t->height += OVERSIZE_AMOUNT; t->width += OVERSIZE_AMOUNT;
        drawFrame(); delay(1);
    }
    for(int i=0; i<3; i++) {
        left_eye.x += MOVEMENT_AMPLITUDE * direction;
        right_eye.x += MOVEMENT_AMPLITUDE * direction;
        right_eye.height += BLINK_AMPLITUDE;
        left_eye.height += BLINK_AMPLITUDE;
        EyeState* t = (direction > 0) ? &right_eye : &left_eye;
        t->height += OVERSIZE_AMOUNT; t->width += OVERSIZE_AMOUNT;
        drawFrame(); delay(1);
    }
    delay(1000);
    for(int i=0; i<3; i++) {
        left_eye.x -= MOVEMENT_AMPLITUDE * direction;
        right_eye.x -= MOVEMENT_AMPLITUDE * direction;    
        right_eye.height -= BLINK_AMPLITUDE;
        left_eye.height -= BLINK_AMPLITUDE;
        EyeState* t = (direction > 0) ? &right_eye : &left_eye;
        t->height -= OVERSIZE_AMOUNT; t->width -= OVERSIZE_AMOUNT;
        drawFrame(); delay(1);
    }
    for(int i=0; i<3; i++) {
        left_eye.x -= MOVEMENT_AMPLITUDE * direction;
        right_eye.x -= MOVEMENT_AMPLITUDE * direction;    
        right_eye.height += BLINK_AMPLITUDE;
        left_eye.height += BLINK_AMPLITUDE;
        EyeState* t = (direction > 0) ? &right_eye : &left_eye;
        t->height -= OVERSIZE_AMOUNT; t->width -= OVERSIZE_AMOUNT;
        drawFrame(); delay(1);
    }
    resetEyes();
}

void EyeAnimation::moveRightBigEye() { moveBigEye(1); }
void EyeAnimation::moveLeftBigEye() { moveBigEye(-1); }

void EyeAnimation::playAnimation(int animationIndex) {
    switch(animationIndex) {
        case WAKEUP: wakeup(); break;
        case RESET: resetEyes(true); break;
        case MOVE_RIGHT_BIG: moveRightBigEye(); break;
        case MOVE_LEFT_BIG: moveLeftBigEye(); break;
        case BLINK_LONG: blink(12); delay(1000); break;
        case BLINK_SHORT: blink(12); break;
        case HAPPY: happyEye(); break;
        case SLEEP: sleep(); break;
        default: break;
    }
}