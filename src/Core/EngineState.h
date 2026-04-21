#pragma once
#include <Arduino.h>
#include <vector>
#include <atomic>

namespace Core {
    struct EngineState {
        // État de l'Interface
        std::atomic<int> activeFaceId{5}; // 5 = RobotEye
        
        // Données Capteurs (Producteurs)
        volatile uint16_t tofGrid[64] = {0};
        std::atomic<float> imuRoll[2]{{0.0f}, {0.0f}};
        std::atomic<float> imuPitch[2]{{0.0f}, {0.0f}};
        volatile float imuAccel[2][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
        volatile float imuGyro[2][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
        volatile float touchStrengths[4] = {0.0f};
        std::atomic<bool> oledShowBars{false};
        
        // Statut Système (Consommateurs)
        std::atomic<bool> buttonPressed{false};
        std::atomic<bool> btConnected{false};
        std::atomic<bool> shouldSendFileList{false};
        std::atomic<uint32_t> oledFrameCounter{0};
        
        // Assets et Config
        std::vector<String> animFiles;
        String sysConfig = "{}";

        // Commandes OLED
        std::atomic<int> oledCommand{-1};
        std::atomic<bool> oledDemoMode{true};

        // Singleton Instance
        static EngineState& instance() {
            static EngineState _instance;
            return _instance;
        }
        
    private:
        EngineState() {} // Constructeur privé
        EngineState(const EngineState&) = delete;
        void operator=(const EngineState&) = delete;
    };
}

// Macro d'accès rapide
#define ENGINE_STATE Core::EngineState::instance()