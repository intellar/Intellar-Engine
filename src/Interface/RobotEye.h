#pragma once
#include <Arduino.h>

namespace Interface {
    class RobotEye {
    public:
        RobotEye();

        // Met à jour la position de l'œil en fonction d'une cible (normalisée -1.0 à 1.0)
        void update(float targetX, float targetY, bool targetValid);

        // Récupère les positions actuelles (normalisées -1.0 à 1.0)
        float getX() const { return _currentX; }
        float getY() const { return _currentY; }

    private:
        float _currentX, _currentY;
        float _targetX, _targetY;
        
        // Pour les saccades (mouvements aléatoires)
        unsigned long _lastSaccadeTime;
        float _saccadeX, _saccadeY;
        
        const float LERP_SPEED = 0.2f;
    };
}