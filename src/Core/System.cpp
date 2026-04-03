#include "System.h"
#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>

namespace Core {
    void init(int sda, int scl) {
        // Initialisation de la communication série pour le debug
        Serial.begin(115200);
        
        // Initialisation unique du bus I2C avec les pins transmises
        Wire.begin(sda, scl);
        Wire.setClock(400000); // 400kHz pour une réactivité optimale
        
        Serial.println(F("--- Intellar Engine Core Initialized ---"));
        Serial.printf("I2C Bus: SDA=%d, SCL=%d @ 400kHz\n", sda, scl);
        psramInit();
    }
}