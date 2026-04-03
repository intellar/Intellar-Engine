#pragma once
#include <stdint.h>
#include "Interface/IntellarModule.h"

namespace Sensors {
    class ToFModule : public Interface::IntellarModule {
    public:
        ToFModule(int lpnPin, int intPin);
        
        bool begin() override;
        void update() override;
        bool isAlive() override { return _connected; }
        String getStatus() override;
        const char* getName() override { return "VL53L5CX"; }

        // Accès statique pour l'orchestrateur
        static ToFModule& instance(int lpn = -1, int intP = -1);

        // Helpers pour la logique de ciblage et debug
        static bool getToFTarget(float &outX, float &outY);
        static void logGrid();

    private:
        int _lpn, _int;
        bool _connected = false;
        static ToFModule* _instance;
    };
}
