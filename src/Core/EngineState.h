#pragma once
#include <Arduino.h>
#include <vector>
#include <atomic>

namespace Core {
    struct EngineState {
        // État de l'Interface
        // 0–4 Chat/sprite strip, 5 RobotEye TFT; défaut Chat neutre (aligné avec setup)
        std::atomic<int> activeFaceId{0};
        // Strip LCD après chargement BLE d’un fichier : -1 = suivre preset (targetCatIndex / 0 gauche), ≥0 = colonne fixe (souvent 0)
        std::atomic<int> lcdExprLeft{-1};
        std::atomic<int> lcdExprRight{-1};
        
        // Données Capteurs (Producteurs)
        volatile uint16_t tofGrid[64] = {0};
        std::atomic<float> imuRoll[2]{{0.0f}, {0.0f}};
        std::atomic<float> imuPitch[2]{{0.0f}, {0.0f}};
        volatile float imuAccel[2][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
        volatile float imuGyro[2][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
        volatile float touchStrengths[4] = {0.0f};
        /** 0 = yeux (anim), 1 = barres touchpad, 2 = écran performance */
        std::atomic<uint8_t> oledUiMode{0};
        
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

        // Diagnostics mode performance OLED (mis à jour ~1 Hz depuis Core 1)
        std::atomic<float> perfDbgFps{0.f};
        std::atomic<uint32_t> perfHeapFree{0};
        std::atomic<uint32_t> perfHeapMin{0};
        std::atomic<uint32_t> perfPsramFree{0};
        std::atomic<uint32_t> perfUptimeSec{0};

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