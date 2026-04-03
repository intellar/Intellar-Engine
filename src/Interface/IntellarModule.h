#pragma once
#include <Arduino.h>

namespace Interface {
    class IntellarModule {
    public:
        virtual ~IntellarModule() {}
        
        // Initialisation du matériel (retourne false si absent)
        virtual bool begin() = 0;
        
        // Mise à jour des données (lecture capteurs)
        virtual void update() = 0;
        
        virtual String getStatus() = 0;
        virtual const char* getName() = 0;
        virtual bool isAlive() { return _connected; }

    protected:
        bool _connected = false;
    };
}