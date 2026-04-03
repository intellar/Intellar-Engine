#include "RobotEye.h"

namespace Interface {
    RobotEye::RobotEye() : _currentX(0), _currentY(0), _targetX(0), _targetY(0), 
                           _lastSaccadeTime(0), _saccadeX(0), _saccadeY(0) {}

    void RobotEye::update(float targetX, float targetY, bool targetValid) {
        float finalTargetX, finalTargetY;

        if (targetValid) {
            // On suit la cible détectée par le ToF
            finalTargetX = targetX;
            finalTargetY = targetY;
        } else {
            // Mode "Idle" : On fait des saccades aléatoires toutes les 1 à 3 secondes
            if (millis() - _lastSaccadeTime > 2000) {
                _lastSaccadeTime = millis();
                _saccadeX = (random(-100, 101) / 100.0f) * 0.8f; // On limite l'amplitude
                _saccadeY = (random(-100, 101) / 100.0f) * 0.8f;
            }
            finalTargetX = _saccadeX;
            finalTargetY = _saccadeY;
        }

        // Interpolation linéaire pour la fluidité
        _currentX += (finalTargetX - _currentX) * LERP_SPEED;
        _currentY += (finalTargetY - _currentY) * LERP_SPEED;
    }
}