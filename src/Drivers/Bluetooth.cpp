#include "Bluetooth.h"
#include "Core/EngineState.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <LittleFS.h>

#define SERVICE_UUID        "590d65c7-3a0a-4023-a05a-6aaf2f22441c"
#define CHARACTERISTIC_UUID "0f0394c8-3162-42da-a0f5-5b0907106037"

static BLECharacteristic* _pCharacteristic = nullptr;
static bool _deviceConnected = false;
static int _lastReceivedFace = -1;

/** Bluetooth animation mapping (Sync with eye_animation.h) */
static const char* OLED_ANIM_NAMES[] = {
  "Wakeup",
  "Reset",
  "Look Right",
  "Look Left",
  "Blink Long",
  "Blink Short",
  "Happy",
  "Sleep",
  "Saccade",
  "Curious",
  "Sad",
  "Bored",
  "Look At"
};
static const int OLED_ANIM_COUNT = sizeof(OLED_ANIM_NAMES) / sizeof(OLED_ANIM_NAMES[0]);

// Callbacks pour gérer la connexion/déconnexion
class ServerCallbacks: public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) {
      _deviceConnected = true;
      // Déclenche l'envoi de la configuration système et de la liste des fichiers
      // dès qu'un appareil se connecte. Cela permet de peupler l'interface 
      // immédiatement sans attendre une commande spécifique.
      ENGINE_STATE.shouldSendFileList.store(true);
    }

    void onDisconnect(BLEServer* pServer) {
      _deviceConnected = false;
      // Redémarrer l'annonce pour permettre une nouvelle connexion immédiate
      pServer->getAdvertising()->start();
    }
};

// Callback pour la réception de données (Write)
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            // Conversion de la chaîne complète en entier (permet "10", "11", etc.)
            _lastReceivedFace = atoi(value.c_str());
        }
    }
};

namespace Drivers {
    namespace Bluetooth {

        void init(const char* deviceName) {
            BLEDevice::init(deviceName);
            BLEDevice::setMTU(517); // Augmenter le MTU pour permettre l'envoi de gros JSON
            BLEServer *pServer = BLEDevice::createServer();
            pServer->setCallbacks(new ServerCallbacks());

            BLEService *pService = pServer->createService(SERVICE_UUID);

            _pCharacteristic = pService->createCharacteristic(
                                CHARACTERISTIC_UUID,
                                BLECharacteristic::PROPERTY_READ |
                                BLECharacteristic::PROPERTY_NOTIFY |
                                BLECharacteristic::PROPERTY_WRITE // Ajout de l'écriture
                            );

            // Ajout du descripteur pour activer les notifications (Client Characteristic Configuration)
            _pCharacteristic->addDescriptor(new BLE2902());
            _pCharacteristic->setCallbacks(new CharacteristicCallbacks());

            pService->start();

            BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
            pAdvertising->addServiceUUID(SERVICE_UUID);
            pAdvertising->setScanResponse(true);
            pAdvertising->setMinPreferred(0x06); 
            pAdvertising->setMinPreferred(0x12);
            BLEDevice::startAdvertising();
        }

        void updateStatus(bool oled, bool lcd_left, bool lcd_right, bool touch, bool imu, bool tof, bool touchpad, int fps_oled, int fps_lcd_l, int fps_lcd_r) {
            if (!_deviceConnected) return;

            // Prioritize file list request over status updates
            if (ENGINE_STATE.shouldSendFileList.load()) {
                // Étape 1 : Configuration système et Modes OLED
                String json = "{\"config\":" + ENGINE_STATE.sysConfig + 
                             ",\"oled_modes\":[{\"id\":80,\"name\":\"Yeux (Anim)\"},{\"id\":81,\"name\":\"Touchpad (Debug)\"}]}";
                _pCharacteristic->setValue(json.c_str());
                _pCharacteristic->notify();
                delay(20);

                // Étape 2 : Liste des personnages (Faces)
                json = "{\"faces\":[";
                json += "{\"id\":0,\"name\":\"Chat Neutre\"},{\"id\":1,\"name\":\"Chat Joyeux\"},";
                json += "{\"id\":2,\"name\":\"Chat Triste\"},{\"id\":3,\"name\":\"Chat Etonne\"},";
                json += "{\"id\":4,\"name\":\"Chat Colere\"}]}";
                _pCharacteristic->setValue(json.c_str());
                _pCharacteristic->notify();
                delay(20);

                // Étape 3 : Liste des animations OLED intégrées
                json = "{\"oled_anims\":[";
                for(int i=0; i<OLED_ANIM_COUNT; i++) {
                    if(i > 0) json += ",";
                    json += "{\"id\":" + String(100 + i) + ",\"name\":\"" + String(OLED_ANIM_NAMES[i]) + "\"}";
                }
                json += "]}";
                _pCharacteristic->setValue(json.c_str());
                _pCharacteristic->notify();
                delay(20);

                // Étape 4 : Liste des fichiers sur la carte SD
                json = "{\"files\":[";
                for(size_t i=0; i<ENGINE_STATE.animFiles.size(); i++) {
                    json += "{\"id\":" + String(10 + i) + ",\"name\":\"" + ENGINE_STATE.animFiles[i] + "\"}";
                    if(i < ENGINE_STATE.animFiles.size() - 1) json += ",";
                }
                json += "]}";
                
                _pCharacteristic->setValue(json.c_str());
                _pCharacteristic->notify();
                ENGINE_STATE.shouldSendFileList.store(false);
                return; 
            }

            char buffer[256];
            // Construction du JSON : {"oled":true,"fps_oled":10,...}
            snprintf(buffer, sizeof(buffer), 
                "{\"oled\":%s,\"lcd_l\":%s,\"lcd_r\":%s,\"touch\":%s,\"imu\":%s,\"tof\":%s,\"touchpad\":%s,\"fps_oled\":%d,\"fps_lcd_l\":%d,\"fps_lcd_r\":%d,\"face\":%d,\"oled_mode\":%d}",
                oled ? "true" : "false",
                lcd_left ? "true" : "false",
                lcd_right ? "true" : "false",
                touch ? "true" : "false",
                imu ? "true" : "false",
                tof ? "true" : "false",
                touchpad ? "true" : "false",
                fps_oled,
                fps_lcd_l,
                fps_lcd_r,
                ENGINE_STATE.activeFaceId.load(), // Utiliser .load() pour obtenir la valeur atomique
                ENGINE_STATE.oledShowBars ? 81 : 80
            );

            _pCharacteristic->setValue(buffer);
            _pCharacteristic->notify();
        }

        void sendPreview(int id, String filename) {
            if (!_deviceConnected) return;
            File file = LittleFS.open(filename, "r");
            if (!file) {
                Serial.printf("ERR: Preview failed to open %s\n", filename.c_str());
                return;
            }

            // Calcul largeur feuille (1200px pour strip 5 visages, sinon calculé)
            size_t fSize = file.size();
            int sheetWidth = (abs((long)fSize - 576000) < 4096) ? 1200 : (fSize / 480);
            if (sheetWidth < 240) sheetWidth = 240;

            // Header spécial pour dire au JS que c'est une image
            // "IMG" + ID (byte) + Width (byte) + Height (byte)
            // On hardcode 120x120 pour le subsample x2
            uint8_t header[6] = {'I', 'M', 'G', (uint8_t)id, 120, 120}; 
            _pCharacteristic->setValue(header, 6);
            _pCharacteristic->notify();
            delay(10);

            // Buffer pour une ligne source (240 px * 2 bytes)
            uint16_t lineBuffer[240];
            // Buffer pour une ligne destination (120 px * 2 bytes)
            uint16_t subBuffer[120];

            // On parcourt les 120 lignes de sortie (correspondant aux lignes paires de l'image source)
            for (int y = 0; y < 120; y++) {
                // Positionnement au début de la ligne source 'y*2'
                // On doit sauter la largeur totale (sheetWidth) pour atteindre la bonne ligne
                size_t offset = (size_t)(y * 2) * (sheetWidth * 2);
                file.seek(offset);

                // Lecture de la ligne source (240 pixels = 480 bytes)
                if (file.read((uint8_t*)lineBuffer, 480) != 480) break;

                // Subsampling : on garde 1 pixel sur 2
                for (int x = 0; x < 120; x++) {
                    subBuffer[x] = lineBuffer[x * 2];
                }

                // Envoi de la ligne subsamplée (240 bytes)
                _pCharacteristic->setValue((uint8_t*)subBuffer, 240);
                _pCharacteristic->notify();
                
                delay(5); // Petit délai pour éviter la congestion BLE
            }
            file.close();
        }

        bool isConnected() {
            return _deviceConnected;
        }

        int getReceivedFace() {
            int face = _lastReceivedFace;
            _lastReceivedFace = -1; // Reset après lecture pour ne pas bloquer
            return face;
        }
    }
}
