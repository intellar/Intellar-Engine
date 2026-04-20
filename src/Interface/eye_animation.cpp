#include "eye_animation.h"
#include "display_wrapper.h"
#include "Core/EngineState.h"

// Import de la fonction définie dans main.cpp
extern void updateAllSensors();

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

void EyeAnimation::curious() {
    resetEyes(false); // État 0 : 40x40px, Y=32

    // --- 1. ANTICIPATION ULTRA-RAPIDE (Le petit plissement, 2 frames) ---
    left_eye.height -= 2; right_eye.height -= 2;
    drawFrame(); delay(10); 

    // --- 2. ACTION SNAPPY (Mouvement accéléré) ---
    // Amplitude réduite, Ratios de conservation de masse (1:3 pour le Squash)
    int steps[] = {3, 7, 10, 12}; // Incréments de ton style
    for(int i = 0; i < 4; i++) {
        int val = steps[i];
        
        // OEIL GAUCHE : S'étire vers le haut (STRETCH)
        left_eye.height = REF_EYE_HEIGHT + val;        // 40 -> 52px
        // On affinede 1px pour 3px de hauteur gagnés pour conserver la masse
        left_eye.width  = REF_EYE_WIDTH - (val / 3);   // 40 -> 36px
        left_eye.y      = (SCREEN_HEIGHT / 2) - (val / 4); // Monte modérément

        // OEIL DROIT : S'écrase vers le bas (SQUASH)
        right_eye.height = REF_EYE_HEIGHT - val;       // 40 -> 28px
        // Il s'élargit massivement pour compenser le volume
        right_eye.width  = REF_EYE_WIDTH + val;        // 40 -> 52px
        right_eye.y      = (SCREEN_HEIGHT / 2) + (val / 4); // Descend modérément

        // Pivot subtil vers la gauche (mouvement global du regard)
        if(i % 2 == 0) { left_eye.x -= 1; right_eye.x -= 1; }

        drawFrame();
        delay(8); // Vitesse "nerveuse"
    }

    // --- 3. MAINTIEN (Pause d'observation intense) ---
    delay(1000); 

    // --- 4. RETOUR NERVEUX (Inverse exact du mouvement) ---
    for(int i = 3; i >= 0; i--) {
        int val = steps[i];
        
        left_eye.height = REF_EYE_HEIGHT + val;
        left_eye.width  = REF_EYE_WIDTH - (val / 3);
        left_eye.y      = (SCREEN_HEIGHT / 2) - (val / 4);

        right_eye.height = REF_EYE_HEIGHT - val;
        right_eye.width  = REF_EYE_WIDTH + val;
        right_eye.y      = (SCREEN_HEIGHT / 2) + (val / 4);

        if(i % 2 == 0) { left_eye.x += 1; right_eye.x += 1; }

        drawFrame();
        delay(12); // Retour un peu plus lent pour simuler le poids mécanique
    }

    resetEyes(true); 
}

void EyeAnimation::sad() {
    resetEyes(false);
    const int STEPS = 12;
    for(int i = 0; i <= STEPS; i++) {
        left_eye.y  = SCREEN_HEIGHT / 2 + (i / 3);
        right_eye.y = SCREEN_HEIGHT / 2 + (i / 3);
        left_eye.height  = REF_EYE_HEIGHT - (i / 3);
        right_eye.height = REF_EYE_HEIGHT - (i / 3);

        g_clear_display();
        drawEyes();

        int eye_top_left  = left_eye.y  - left_eye.height  / 2;
        int eye_top_right = right_eye.y - right_eye.height / 2;

        // Masque côté EXTÉRIEUR pour un air triste/ennuyé (V inversé)
        g_draw_filled_triangle(
            left_eye.x - left_eye.width / 2,  eye_top_left,
            left_eye.x - left_eye.width / 2,  eye_top_left + i,
            left_eye.x,                        eye_top_left,
            G_COLOR_BLACK);

        g_draw_filled_triangle(
            right_eye.x + right_eye.width / 2, eye_top_right,
            right_eye.x + right_eye.width / 2, eye_top_right + i,
            right_eye.x,                        eye_top_right,
            G_COLOR_BLACK);

        g_update_display();
        delay(20);
    }
    delay(1500);
    resetEyes(true);
}


void EyeAnimation::bored() {
    resetEyes(false);
    float smoothL = 0, smoothR = 0;
    float wakefulness = 0.0f;
    unsigned long lastBlinkTime = millis();
    unsigned long nextBlinkInterval = 2000 + random(3000);

    while (ENGINE_STATE.oledCommand == BORED || ENGINE_STATE.oledCommand == -1) {
        if (ENGINE_STATE.oledCommand != -1 && ENGINE_STATE.oledCommand != BORED) break;

        // Permettre aux IMU de se mettre à jour pendant l'animation
        updateAllSensors();

        float rawL = max(ENGINE_STATE.touchStrengths[0], ENGINE_STATE.touchStrengths[1]);
        float rawR = max(ENGINE_STATE.touchStrengths[2], ENGINE_STATE.touchStrengths[3]);

        float targetL = constrain(rawL * 4.0f, 0.0f, 1.0f);
        float targetR = constrain(rawR * 4.0f, 0.0f, 1.0f);

        float inputActivity = max(targetL, targetR);
        // Éveil plus lent à monter, plus rapide à descendre
        if (inputActivity > 0.05f) wakefulness += inputActivity * 0.008f;
        else                       wakefulness -= 0.006f;
        wakefulness = constrain(wakefulness, 0.0f, 1.0f);

        float wake = max(max(targetL, targetR), wakefulness);
        smoothL = smoothL * 0.85f + max(targetL, wakefulness) * 0.15f;
        smoothR = smoothR * 0.85f + max(targetR, wakefulness) * 0.15f;

        // GÉOMÉTRIE : hauteur de 5px (très endormi) à REF_EYE_HEIGHT (éveillé)
        left_eye.height  = 5 + (int)(smoothL * (REF_EYE_HEIGHT - 5));
        right_eye.height = 5 + (int)(smoothR * (REF_EYE_HEIGHT - 5));

        // Les yeux s'élargissent légèrement en s'aplatissant (conservation masse)
        left_eye.width  = REF_EYE_WIDTH + (int)((1.0f - smoothL) * 8);
        right_eye.width = REF_EYE_WIDTH + (int)((1.0f - smoothR) * 8);

        // Les yeux tombent vers le bas quand fatigués
        left_eye.y  = SCREEN_HEIGHT / 2 + (int)((1.0f - smoothL) * 8);
        right_eye.y = SCREEN_HEIGHT / 2 + (int)((1.0f - smoothR) * 8);

        // Corner radius proportionnel à la hauteur
        corner_radius = map(left_eye.height, 5, REF_EYE_HEIGHT, 1, REF_CORNER_RADIUS);
        corner_radius  = min(corner_radius, left_eye.height / 2);

        g_clear_display();
        drawEyes();

        // PAUPIÈRE LOURDE : deux triangles noirs, intérieur ET extérieur
        // Intensité inversée : plus l'œil est fermé, plus le masque est grand
        int maskL = (int)((1.0f - smoothL) * 18);
        int maskR = (int)((1.0f - smoothR) * 18);
        int topL = left_eye.y  - left_eye.height / 2;
        int topR = right_eye.y - right_eye.height / 2;

        // Masque extérieur gauche (coin gauche de l'œil gauche)
        if (maskL > 2) {
            g_draw_filled_triangle(
                left_eye.x - left_eye.width/2, topL,
                left_eye.x - left_eye.width/2, topL + maskL,
                left_eye.x + left_eye.width/4, topL,
                G_COLOR_BLACK);
        }
        // Masque intérieur gauche (coin droit de l'œil gauche)
        if (maskL > 4) {
            g_draw_filled_triangle(
                left_eye.x + left_eye.width/2, topL,
                left_eye.x + left_eye.width/2, topL + maskL/2,
                left_eye.x - left_eye.width/4, topL,
                G_COLOR_BLACK);
        }
        // Masque extérieur droit
        if (maskR > 2) {
            g_draw_filled_triangle(
                right_eye.x + right_eye.width/2, topR,
                right_eye.x + right_eye.width/2, topR + maskR,
                right_eye.x - right_eye.width/4, topR,
                G_COLOR_BLACK);
        }
        // Masque intérieur droit
        if (maskR > 4) {
            g_draw_filled_triangle(
                right_eye.x - right_eye.width/2, topR,
                right_eye.x - right_eye.width/2, topR + maskR/2,
                right_eye.x + right_eye.width/4, topR,
                G_COLOR_BLACK);
        }

        g_update_display();

        // Micro-clignement aléatoire quand très endormi
        if (wakefulness < 0.2f && millis() - lastBlinkTime > nextBlinkInterval) {
            blink(14);
            lastBlinkTime = millis();
            nextBlinkInterval = 3000 + random(4000);
        }

        delay(16);
    }
    resetEyes(true);
}
void EyeAnimation::lookat() {
    resetEyes(false);
    float smoothX = 0, smoothY = 0;
    float lastSmoothX = 0;
    const int MAX_MOVE_X = 16;
    const int MAX_MOVE_Y = 6;

    while (ENGINE_STATE.oledCommand == LOOKAT || ENGINE_STATE.oledCommand == -1) {
        if (ENGINE_STATE.oledCommand != -1 && ENGINE_STATE.oledCommand != LOOKAT) break;

        // Permettre aux IMU de se mettre à jour pendant l'animation
        updateAllSensors();

        float rawL = max(ENGINE_STATE.touchStrengths[0], ENGINE_STATE.touchStrengths[1]);
        float rawR = max(ENGINE_STATE.touchStrengths[2], ENGINE_STATE.touchStrengths[3]);

        float targetX = constrain((rawR - rawL) * 4.0f, -1.0f, 1.0f);
        float activity = constrain(max(rawL, rawR) * 4.0f, 0.0f, 1.0f);

        // Lerp plus rapide quand activité forte
        float lerpFactor = 0.06f + activity * 0.18f;
        lastSmoothX = smoothX;
        smoothX = smoothX + (targetX - smoothX) * lerpFactor;

        // Léger mouvement Y : les yeux montent quand on regarde fort
        float targetY = -activity * 0.4f;
        smoothY = smoothY * 0.88f + targetY * 0.12f;

        float velocity = abs(smoothX - lastSmoothX);
        // Squash plafonné — max 12px pour rester lisible
        int squashAmount = min((int)(velocity * 30), 12);
        int oversizeAmount = (int)(activity * 5);

        // Positions de base + décalage
        int baseLX = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
        int baseRX = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;
        int baseY  = SCREEN_HEIGHT / 2;
        int movePx = (int)(smoothX * MAX_MOVE_X);
        int moveYPx = (int)(smoothY * MAX_MOVE_Y);

        left_eye.x  = baseLX + movePx;
        right_eye.x = baseRX + movePx;
        left_eye.y  = baseY + moveYPx;
        right_eye.y = baseY + moveYPx;

        if (smoothX > 0.1f) {
            // Regarde droite : œil droit mène (plus grand, plus haut)
            right_eye.height = REF_EYE_HEIGHT + oversizeAmount - squashAmount;
            right_eye.width  = REF_EYE_WIDTH  + oversizeAmount;
            left_eye.height  = REF_EYE_HEIGHT - squashAmount;
            left_eye.width   = REF_EYE_WIDTH;
        } else if (smoothX < -0.1f) {
            // Regarde gauche
            left_eye.height  = REF_EYE_HEIGHT + oversizeAmount - squashAmount;
            left_eye.width   = REF_EYE_WIDTH  + oversizeAmount;
            right_eye.height = REF_EYE_HEIGHT - squashAmount;
            right_eye.width  = REF_EYE_WIDTH;
        } else {
            // Neutre — léger squash symétrique si velocity
            left_eye.height = right_eye.height = REF_EYE_HEIGHT - squashAmount;
            left_eye.width  = right_eye.width  = REF_EYE_WIDTH;
        }

        // Clamp sécurité
        left_eye.height  = max(left_eye.height,  4);
        right_eye.height = max(right_eye.height, 4);

        // Corner radius proportionnel à la hauteur minimale des deux yeux
        int minH = min(left_eye.height, right_eye.height);
        corner_radius = map(minH, 4, REF_EYE_HEIGHT + 6, 1, REF_CORNER_RADIUS);
        corner_radius = min(corner_radius, minH / 2);

        drawFrame();
        delay(16);
    }
    resetEyes(true);
}
/*
void EyeAnimation::bored_() {
    resetEyes(false);
    float smoothL = 0, smoothR = 0;
    float wakefulness = 0.0f; 
    
    // On boucle tant que la commande est active
    while (ENGINE_STATE.oledCommand == 11 || ENGINE_STATE.oledCommand == -1) {
        if (ENGINE_STATE.oledCommand != -1 && ENGINE_STATE.oledCommand != 11) break;

        // Lecture des capteurs avec ton gain de 2.5f
        float rawL = max(ENGINE_STATE.touchStrengths[0], ENGINE_STATE.touchStrengths[1]);
        float rawR = max(ENGINE_STATE.touchStrengths[2], ENGINE_STATE.touchStrengths[3]);
        
        float targetL = constrain(rawL * 4.0f, 0.0f, 1.0f);
        float targetR = constrain(rawR * 4.0f, 0.0f, 1.0f);
        
        // Mise à jour de la mémoire d'éveil
        float inputActivity = max(targetL, targetR);
        if (inputActivity > 0.05f) {
            wakefulness += inputActivity * 0.015f; // Légèrement plus rapide à l'éveil
        } else {
            wakefulness -= 0.01f; // S'endort doucement
        }
        wakefulness = constrain(wakefulness, 0.0f, 1.0f);

        // Lissage
        smoothL = smoothL * 0.82f + max(targetL, wakefulness) * 0.18f;
        smoothR = smoothR * 0.82f + max(targetR, wakefulness) * 0.18f;

        // GÉOMÉTRIE : Yeux lourds
        // Hauteur de 6 (plus visible que 4) à 36px
        left_eye.height = 6 + (int)(smoothL * 30); 
        right_eye.height = 6 + (int)(smoothR * 30);
        
        // Déplacement vertical : les yeux "tombent" quand ils sont fatigués
        left_eye.y = (SCREEN_HEIGHT / 2) + 6 - (int)(smoothL * 10);
        right_eye.y = (SCREEN_HEIGHT / 2) + 6 - (int)(smoothR * 10);

        g_clear_display();
        drawEyes();

        // APPLICATION DU MASQUE "PAUPIÈRE LOURDE"
        // On utilise tes triangles noirs pour couper le haut extérieur
        int maskIntensityL = (int)((1.0f - smoothL) * 16);
        int maskIntensityR = (int)((1.0f - smoothR) * 16);

        int topL = left_eye.y - left_eye.height / 2;
        int topR = right_eye.y - right_eye.height / 2;

        if(maskIntensityL > 2) {
            g_draw_filled_triangle(
                left_eye.x - left_eye.width / 2, topL,
                left_eye.x - left_eye.width / 2, topL + maskIntensityL,
                left_eye.x + (left_eye.width / 4), topL, // Coupe plus large pour l'ennui
                G_COLOR_BLACK);
        }
        if(maskIntensityR > 2) {
            g_draw_filled_triangle(
                right_eye.x + right_eye.width / 2, topR,
                right_eye.x + right_eye.width / 2, topR + maskIntensityR,
                right_eye.x - (right_eye.width / 4), topR, 
                G_COLOR_BLACK);
        }

        g_update_display();
        delay(16); 
    }
    resetEyes(true);
}

void EyeAnimation::lookat() {
    resetEyes(false);
    float smoothX = 0;
    float lastSmoothX = 0;
    const int MAX_MOVE_X = 18; // Amplitude maximale du regard
    
    // On boucle tant que la commande est active (Index 12 pour LOOKAT)
    while (ENGINE_STATE.oledCommand == 12 || ENGINE_STATE.oledCommand == -1) {
        if (ENGINE_STATE.oledCommand != -1 && ENGINE_STATE.oledCommand != 12) break;

        float rawL = max(ENGINE_STATE.touchStrengths[0], ENGINE_STATE.touchStrengths[1]);
        float rawR = max(ENGINE_STATE.touchStrengths[2], ENGINE_STATE.touchStrengths[3]);
        
        // Direction cible : de -1.0 (gauche) à 1.0 (droite)
        float targetX = (rawR - rawL) * 4.0f; 
        targetX = constrain(targetX, -1.0f, 1.0f);

        // Calcul de l'activité globale et de la vélocité du regard
        float activity = max(rawL, rawR);
        float lerpFactor = 0.04f + (activity * 0.20f); 
        
        lastSmoothX = smoothX;
        smoothX = smoothX + (targetX - smoothX) * lerpFactor;
        
        // La vélocité détermine l'écrasement (blink-on-move)
        float velocity = abs(smoothX - lastSmoothX);

        // Reset des paramètres de base pour recalculer le mouvement
        resetEyes(false);
        int movePx = (int)(smoothX * MAX_MOVE_X);
        left_eye.x += movePx;
        right_eye.x += movePx;

        // MÉCANIQUE "MOVE BIG EYE" APPLIQUÉE :
        // 1. Oversize : L'œil directeur devient plus gros selon l'appui
        // 2. Squash : Les deux yeux s'aplatissent proportionnellement à la vitesse
        int oversizeAmount = (int)(activity * 6);
        int squashAmount = (int)(velocity * 45); // Écrasement pendant les saccades

        if (smoothX > 0.1f) { // Regarde à droite : l'œil droit mène
            right_eye.height = REF_EYE_HEIGHT + oversizeAmount - squashAmount;
            right_eye.width  = REF_EYE_WIDTH + oversizeAmount;
            left_eye.height  = REF_EYE_HEIGHT - squashAmount;
        } 
        else if (smoothX < -0.1f) { // Regarde à gauche : l'œil gauche mène
            left_eye.height  = REF_EYE_HEIGHT + oversizeAmount - squashAmount;
            left_eye.width   = REF_EYE_WIDTH + oversizeAmount;
            right_eye.height = REF_EYE_HEIGHT - squashAmount;
        } 
        else { // Neutre
            left_eye.height = right_eye.height = REF_EYE_HEIGHT - squashAmount;
        }

        // Sécurité pour ne pas avoir une hauteur négative
        if (left_eye.height < 4) left_eye.height = 4;
        if (right_eye.height < 4) right_eye.height = 4;

        drawFrame();
        delay(16); // ~60 FPS
    }
    resetEyes(true);
}
*/
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
        case SACCADE_RANDOM: saccade(random(-1,2), random(-1,2)); break;
        case CURIOUS: curious(); break;
        case SAD: sad(); break;
        case BORED: bored(); break;
        case LOOKAT: lookat(); break;
        default: break;
    }
}