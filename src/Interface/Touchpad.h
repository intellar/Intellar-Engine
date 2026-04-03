#pragma once
#include <stdint.h>

namespace Drivers {
    namespace Touchpad {
        // Initialise le module Touchpad et calibre la baseline
        void init();
        
        // Lit les capteurs et met à jour les états (à appeler dans loop)
        void update();
        
        // Retourne true si la touche à l'index donné (0 à 3) est active
        bool isTouched(uint8_t index);
        
        // Retourne la valeur brute de capacitance (pour debug)
        int getValue(uint8_t index);

        // Retourne la force du signal normalisée entre 0.0 et 1.0 (sans seuil)
        float getStrength(uint8_t index);

        // Retourne la valeur minimale observée (pour debug)
        int getMin(uint8_t index);

        // Retourne la valeur maximale observée (pour debug)
        int getMax(uint8_t index);

        // Retourne true si une interaction a été détectée depuis le démarrage (pour le statut système)
        bool isDetected();
    }
}
