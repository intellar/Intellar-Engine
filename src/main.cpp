#include <Arduino.h>
#include "Core/System.h"
#include "Core/EngineState.h"
#include "Sensors/IMU.h"
#include "Sensors/ToF.h"
#include "Drivers/OLED.h"
#include "Drivers/LCD.h"
#include "Drivers/Bluetooth.h"
#include "Interface/Touchpad.h"
#include "Interface/RobotEye.h"
#include "Interface/display_wrapper.h"
#include "Interface/eye_animation.h"
#include <SPI.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <LittleFS.h>
#include <vector>

// --- Configuration Hardware et Hub ---
static std::vector<Interface::IntellarModule*> i2cModules;

#if !defined(TFT_DC) || !defined(TFT_RST)
    #ifndef UNIT_TEST // Optionnel : ignorer pour les tests unitaires
        #warning "TFT pins not detected in platformio.ini. Ensure build_flags are correctly configured."
    #endif
#endif

#ifndef TFT_LED
    #ifdef TFT_BL
        #define TFT_LED TFT_BL
    #else
        #define TFT_LED 255 // Default value for systems without hardware backlight control
    #endif
#endif

// --- Paramètres de synchronisation ---
static const int TARGET_FPS = 30;
static const int FRAME_DELAY_MS = 1000 / TARGET_FPS;
static const int TOUCH_UPDATE_MS = 50;

const int BL_PWM_CHANNEL = 0;
const int BL_PWM_FREQ = 5000;
const int BL_PWM_RESOLUTION = 8;

EyeAnimation eyeAnim;
Interface::RobotEye robotEyeLogic;

bool isDevicePresent(uint8_t address) {
    Wire.beginTransmission(address);
    return (Wire.endTransmission() == 0);
}

/** Dedicated task for I2C communication (ToF, OLED, Sensors) running on Core 0 */
void i2cTask(void *pvParameters) {
  Serial.println("INFO: Tâche I2C (IMU+OLED) démarrée sur le Coeur 0.");

  // On vérifie la présence de l'OLED (0x3C) une seule fois
  bool oledEnabled = isDevicePresent(0x3C);
  if (oledEnabled) {
      eyeAnim.begin();
  }
  
  static int demo_anim_index = 0;
  unsigned long lastAnimTime = millis();
  unsigned long nextInterval = 3000; 

  // Define fixed periodicity for the I2C task (50Hz)
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(20);

  for (;;) {
    // Synchronized peripheral updates
    for (auto module : i2cModules) {
        if (module->isAlive()) {
            module->update();
        }
    }

    if (ENGINE_STATE.oledCommand != -1) {
        if (oledEnabled) eyeAnim.playAnimation(ENGINE_STATE.oledCommand);
        ENGINE_STATE.oledCommand = -1;
        lastAnimTime = millis();
        nextInterval = 3000;
    }
    // Automatic demo cycle
    else if (millis() - lastAnimTime > nextInterval) {
        if (oledEnabled) eyeAnim.playAnimation(demo_anim_index);
        demo_anim_index = (demo_anim_index + 1) % MAX_ANIMATIONS;
        lastAnimTime = millis();
        nextInterval = 1000;
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup() {
  Core::init(I2C_SDA, I2C_SCL);
  Serial.begin(115200);

  if(!LittleFS.begin()){
      Serial.println("ERREUR: Montage LittleFS échoué !");
  } else {
      Serial.println("INFO: LittleFS monté avec succès.");
      File root = LittleFS.open("/");
      File file = root.openNextFile();
      while(file){
          String fname = file.name();
          if(fname.endsWith(".bin") && fname.indexOf("expression-") >= 0) {
              if (!fname.startsWith("/")) fname = "/" + fname;
              ENGINE_STATE.animFiles.push_back(fname);
              Serial.printf("INFO: Animation trouvee: %s\n", fname.c_str());
          }
          file = root.openNextFile();
      }
  }

  // Initialize I2C modules (ToF instantiation can be enabled here)
  for (auto m : i2cModules) {
      if (!m->begin()) Serial.println("ERROR: Echec initialisation d'un module I2C");
  }
  Wire.setClock(400000); 

  Drivers::initLCD(TFT_CS, TFT_DC, TFT_RST, TFT_LED);
  Drivers::loadRobotEyeRes("/image_giant.bin");
  
  ENGINE_STATE.activeFaceId = 0; 

  if (ENGINE_STATE.animFiles.size() >= 1) {
      Drivers::setAnimation(ENGINE_STATE.animFiles[0].c_str(), Drivers::DisplayIndex::LEFT);
  }
  if (ENGINE_STATE.animFiles.size() >= 2) {
      Drivers::setAnimation(ENGINE_STATE.animFiles[1].c_str(), Drivers::DisplayIndex::RIGHT);
  }

  Drivers::Touchpad::init();
  
  // Generate static JSON system manifest
  ENGINE_STATE.sysConfig = "{";

  #if defined(HAS_OLED)
    ENGINE_STATE.sysConfig += "\"oled\":true,";
  #else
    ENGINE_STATE.sysConfig += "\"oled\":false,";
  #endif

  #if defined(HAS_IMU)
    ENGINE_STATE.sysConfig += "\"imu\":true,";
  #else
    ENGINE_STATE.sysConfig += "\"imu\":false,";
  #endif

  #if defined(SCREEN_GC9A01_DUAL)
    ENGINE_STATE.sysConfig += "\"lcd_l\":true,\"lcd_r\":true,";
  #elif defined(SCREEN_GC9A01) || defined(SCREEN_ILI9341)
    ENGINE_STATE.sysConfig += "\"lcd_l\":false,\"lcd_r\":true,";
  #else
    ENGINE_STATE.sysConfig += "\"lcd_l\":false,\"lcd_r\":false,";
  #endif

  #if defined(TOUCH_CS)
    ENGINE_STATE.sysConfig += "\"touch\":true,";
  #else
    ENGINE_STATE.sysConfig += "\"touch\":false,";
  #endif

  #if defined(HAS_TOF)
    ENGINE_STATE.sysConfig += "\"tof\":true,";
  #else
    ENGINE_STATE.sysConfig += "\"tof\":false,";
  #endif

  ENGINE_STATE.sysConfig += "\"touchpad\":true";
  ENGINE_STATE.sysConfig += "}";

  Drivers::Bluetooth::init("Intellar-Engine");

  delay(100);

  xTaskCreatePinnedToCore(
      i2cTask, "I2C Task", 8192, NULL, 4, NULL, 0);
}

void loop() {
  static int frameCount = 0;
  static unsigned long lastFpsTime = 0;
  static unsigned long lastTouchUpdate = 0;
  static unsigned long lastRefresh = 0;
  
  unsigned long now = millis();

  ENGINE_STATE.btConnected = Drivers::Bluetooth::isConnected();
  
  if (now - lastTouchUpdate >= TOUCH_UPDATE_MS) {
      Drivers::Touchpad::update();
      for(int i=0; i<4; i++) {
          ENGINE_STATE.touchStrengths[i] = Drivers::Touchpad::getStrength(i);
      }
      lastTouchUpdate = now;
  }

  static int persistentFace = 0;
  int targetCatIndex = persistentFace;
  int targetSingeIndex = 0;

  if (Drivers::Touchpad::isTouched(0)) { targetCatIndex = 1; targetSingeIndex = 1; }
  else if (Drivers::Touchpad::isTouched(1)) { targetCatIndex = 2; targetSingeIndex = 2; }
  else if (Drivers::Touchpad::isTouched(2)) { targetCatIndex = 4; targetSingeIndex = 4; }
  else if (Drivers::Touchpad::isTouched(3)) { targetCatIndex = 3; targetSingeIndex = 3; }

  int btCmd = Drivers::Bluetooth::getReceivedFace();
  if (btCmd != -1 && btCmd >= 0 && btCmd <= 5) {
      persistentFace = btCmd;
      targetCatIndex = btCmd;
      ENGINE_STATE.activeFaceId = btCmd;
      Serial.printf("CMD BT: Face ID changed to %d\n", btCmd);
  }

  if (btCmd == 99) ENGINE_STATE.shouldSendFileList = true;
  else if (btCmd >= 100 && btCmd < 200) ENGINE_STATE.oledCommand = btCmd - 100;
  else if (btCmd >= 200 && btCmd < 300) {
      int idx = btCmd - 200;
      if (idx < ENGINE_STATE.animFiles.size()) Drivers::Bluetooth::sendPreview(10 + idx, ENGINE_STATE.animFiles[idx]);
  }
  else if (btCmd >= 30 && btCmd < 50) {
      int idx = btCmd - 30;
      if (idx < ENGINE_STATE.animFiles.size()) Drivers::setAnimation(ENGINE_STATE.animFiles[idx].c_str(), Drivers::DisplayIndex::RIGHT);
  }
  else if (btCmd >= 10 && btCmd < 30) {
      int idx = btCmd - 10;
      if (idx < ENGINE_STATE.animFiles.size()) Drivers::setAnimation(ENGINE_STATE.animFiles[idx].c_str(), Drivers::DisplayIndex::LEFT);
  }

  // Main display refresh logic (Target: 30 FPS)
  if (lastRefresh == 0) lastRefresh = now;

  if (now - lastRefresh >= FRAME_DELAY_MS) { 
      lastRefresh += FRAME_DELAY_MS; 

      if (ENGINE_STATE.activeFaceId == 5) {
          float tx_eye, ty_eye;
          bool valid = Sensors::ToFModule::getToFTarget(tx_eye, ty_eye);
          robotEyeLogic.update(tx_eye, ty_eye, valid);
          Drivers::showRobotEyes(robotEyeLogic.getX(), robotEyeLogic.getY(), (const uint16_t*)ENGINE_STATE.tofGrid);
      } else if (ENGINE_STATE.activeFaceId >= 0 && ENGINE_STATE.activeFaceId <= 4) {
          Drivers::showCatFace(targetCatIndex, targetSingeIndex);
      } else {
          Drivers::updateLCD();
      }
      frameCount++;
  }

  delay(5); 

  if (now - lastFpsTime >= 1000) {
    float fps = frameCount * 1000.0f / (now - lastFpsTime);
    Serial.printf("System Real FPS: %.1f\n", fps);
    
    // Grid analysis for diagnostics
    int validPoints = 0;
    uint32_t sumDist = 0;
    for(int i=0; i<64; i++) {
        if(ENGINE_STATE.tofGrid[i] > 0 && ENGINE_STATE.tofGrid[i] < 4000) {
            validPoints++;
            sumDist += ENGINE_STATE.tofGrid[i];
        }
    }

    if (Sensors::ToFModule::instance().isAlive()) {
        Serial.printf("ToF Status: [OK] | Pts Valides: %d | Moyenne: %u mm | Centre: %d mm\n", 
                      validPoints, validPoints > 0 ? sumDist/validPoints : 0, ENGINE_STATE.tofGrid[28]);
        Serial.printf("ToF Raw Grid (0,1,2,3,4,5,6,7): %d, %d, %d, %d, %d, %d, %d, %d\n",
                      ENGINE_STATE.tofGrid[0], ENGINE_STATE.tofGrid[1],
                      ENGINE_STATE.tofGrid[2], ENGINE_STATE.tofGrid[3],
                      ENGINE_STATE.tofGrid[4], ENGINE_STATE.tofGrid[5],
                      ENGINE_STATE.tofGrid[6], ENGINE_STATE.tofGrid[7]);
    } else {
        Serial.println("ToF Status: [ERROR] Capteur absent ou firmware non charge");
    }

    uint32_t currentOledCounter = ENGINE_STATE.oledFrameCounter;
    static uint32_t lastOledCounter = 0;
    int actual_oled_fps = (int)(currentOledCounter - lastOledCounter);
    lastOledCounter = currentOledCounter;

    bool st_oled = (actual_oled_fps > 0);
    bool st_lcd_l = false, st_lcd_r = false, st_touch = false, st_imu = false;
    bool st_tof = Sensors::ToFModule::instance().isAlive(); 
    bool st_touchpad = Drivers::Touchpad::isDetected(); 
    
    int fps_lcd_l = 0;
    int fps_lcd_r = 0;
    int current_fps = (int)fps; 

#if defined(SCREEN_GC9A01_DUAL)
    st_lcd_l = true; st_lcd_r = true;
    fps_lcd_l = current_fps; fps_lcd_r = current_fps;
#elif defined(SCREEN_GC9A01) || defined(SCREEN_ILI9341)
    st_lcd_r = true; fps_lcd_r = current_fps;
#endif

    Drivers::Bluetooth::updateStatus(st_oled, st_lcd_l, st_lcd_r, st_touch, st_imu, st_tof, st_touchpad, actual_oled_fps, fps_lcd_l, fps_lcd_r);

    frameCount = 0;
    lastFpsTime = now;
  }
}