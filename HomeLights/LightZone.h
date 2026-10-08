#pragma once

#include <Arduino.h>
#include "config.h"

enum LightZoneId : uint8_t { ZONE_CUNA = 0, ZONE_SETUP = 1, ZONE_COUNT = 2 };

class LightZone {
 public:
  explicit LightZone(uint8_t gpio);

  void begin();
  void update();

  void setLightLevel(uint8_t percent);
  void setLightOn();
  void setLightOff();
  void fadeLight(uint8_t targetPercent, uint32_t durationMs);
  void toggleLight();

  uint8_t currentLevel() const;
  uint8_t targetLevel() const;
  float brightnessPercent() const { return brightness_ * 100.0f; }
  /** Brillo físico > 0; usar phaseName() para OFF/ON/transiciones. */
  bool isOn() const { return brightness_ > 0.002f; }
  bool isTransitioning() const { return fadeActive_; }
  /** "off" | "turning_on" | "on" | "turning_off" */
  const char* phaseName() const;

 private:
  void applyPwmFromBrightness(float linear0to1);
  void updateFade();
  static float percentToBrightness(uint8_t percent);

  uint8_t gpio_;

  float brightness_ = 0.0f;

  bool fadeActive_ = false;
  uint32_t fadeStartMs_ = 0;
  uint32_t fadeDurationMs_ = 0;
  float fadeStartBright_ = 0.0f;
  float fadeTargetBright_ = 0.0f;
};
