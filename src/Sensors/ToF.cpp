#include "ToF.h"
#include "Core/EngineState.h"
#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>

static SparkFun_VL53L5CX myImager;
static VL53L5CX_ResultsData measurementData; // Structure pour stocker les résultats

namespace Sensors {

    ToFModule* ToFModule::_instance = nullptr;

    ToFModule& ToFModule::instance(int lpn, int intP) {
        if (!_instance) _instance = new ToFModule(lpn, intP);
        return *_instance;
    }

    ToFModule::ToFModule(int lpnPin, int intPin) : _lpn(lpnPin), _int(intPin) {}

    bool ToFModule::begin() {
        Serial.println("ToF: Starting hardware initialization...");
        if (_lpn >= 0) {
            pinMode(_lpn, OUTPUT);
            digitalWrite(_lpn, LOW);
            delay(100);
            digitalWrite(_lpn, HIGH);
            delay(600); 
        }

        Wire.setTimeOut(100);
        Wire.setClock(100000);

        if (myImager.begin(0x29, Wire) == false) {
            Serial.println("ToF: I2C communication error or firmware load failure");
            _connected = false;
            return false;
        } else {
            myImager.setWireMaxPacketSize(128); 
            _connected = true;
            Wire.setClock(400000);
            
            myImager.setResolution(8*8);
            myImager.setRangingFrequency(15);
            if (myImager.startRanging()) Serial.println("ToF: Ranging started successfully");
            return true;
        }
    }

    void ToFModule::update() {
        if (!_connected) return;
        
        if (myImager.isDataReady()) { 
            if (myImager.getRangingData(&measurementData)) {
                for (int i = 0; i < 64; i++) {
                    ENGINE_STATE.tofGrid[i] = measurementData.distance_mm[i];
                }
            }
        }
    }

    String ToFModule::getStatus() {
        if (!_connected) return "OFFLINE";
        return "OK - " + String(ENGINE_STATE.tofGrid[28]) + "mm";
    }

    bool ToFModule::getToFTarget(float &outX, float &outY) {
        uint16_t minDist = 2000;
        int targetIdx = -1;
        for (int i = 0; i < 64; i++) {
            if (ENGINE_STATE.tofGrid[i] > 50 && ENGINE_STATE.tofGrid[i] < minDist) {
                minDist = ENGINE_STATE.tofGrid[i];
                targetIdx = i;
            }
        }
        if (targetIdx != -1 && minDist < 1000) {
            int tx = targetIdx % 8;
            int ty = targetIdx / 8;
            outX = (tx - 3.5f) / 4.0f;
            outY = (ty - 3.5f) / 4.0f;
            return true;
        }
        return false;
    }

    void ToFModule::logGrid() {
        Serial.println("\n--- ToF Grid 8x8 (mm) ---");
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                // Coordinate inversion for natural visual mapping
                int idx = (7 - y) * 8 + x; 
                uint16_t dist = ENGINE_STATE.tofGrid[idx];
                if (dist > 2000) Serial.print(" --  ");
                else Serial.printf("%4d ", dist);
            }
            Serial.println();
        }
        Serial.println("-------------------------\n");
    }
}
