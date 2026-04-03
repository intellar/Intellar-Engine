#pragma once
#include <Arduino.h>

namespace Drivers {
    namespace Bluetooth {
        // Initialise le serveur BLE avec un nom de périphérique
        void init(const char* deviceName);

        // Envoie l'état des modules connectés aux clients abonnés (via Notify)
        void updateStatus(bool oled, bool lcd_left, bool lcd_right, bool touch, bool imu, bool tof, bool touchpad, int fps_oled, int fps_lcd_l, int fps_lcd_r);

        // Envoie une miniature de la 1ère frame du fichier (Subsample 2x : 120x120)
        void sendPreview(int id, String filename);

        // Retourne l'état de la connexion
        bool isConnected();
        
        // Récupère l'index du visage demandé (si reçu), sinon retourne -1
        int getReceivedFace();
    }
}
