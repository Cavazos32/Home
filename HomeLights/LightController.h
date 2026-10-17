#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

enum LightZoneId : uint8_t { ZONE_CUNA = 0, ZONE_SETUP = 1, ZONE_COUNT = 2 };

#ifndef MAX_ROUTINE_GROUPS
#define MAX_ROUTINE_GROUPS 6
#endif
#ifndef MAX_ROUTINE_STEPS
#define MAX_ROUTINE_STEPS 4
#endif

enum RoutineId : uint8_t {
  ROUTINE_NONE = 0,
  ROUTINE_HABITACION = 1,
  ROUTINE_BABY = 2,
  ROUTINE_COUNT = MAX_ROUTINE_GROUPS
};

struct LightSettings {
  uint32_t fadeOnMs = FADE_ON_MS;
  uint32_t fadeOffMs = FADE_OFF_MS;
  float fadeGamma = FADE_GAMMA;
  uint8_t defaultOnLevel = DEFAULT_ON_LEVEL;
};

class LightZone {
 public:
  explicit LightZone(uint8_t gpio);

  void bindSettings(const LightSettings* settings);
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
  bool isOn() const { return brightness_ > 0.002f; }
  bool isLit() const;
  bool isTransitioning() const { return fadeActive_; }
  float transitionProgress() const;
  const char* phaseName() const;

 private:
  void applyPwmFromBrightness(float linear0to1);
  void updateFade();
  static float percentToBrightness(uint8_t percent);

  uint8_t gpio_;
  const LightSettings* settings_ = nullptr;
  float brightness_ = 0.0f;
  bool fadeActive_ = false;
  uint32_t fadeStartMs_ = 0;
  uint32_t fadeDurationMs_ = 0;
  float fadeStartBright_ = 0.0f;
  float fadeTargetBright_ = 0.0f;
};

class LightController {
 public:
  LightController();
  ~LightController();

  LightController(const LightController&) = delete;
  LightController& operator=(const LightController&) = delete;

  void begin();
  void update();
  void startClock();
  bool timeSynced() const;
  String currentTimeText();

  void onFirmwareUpdateStart();

  LightZone& zone(LightZoneId id);
  const LightZone& zone(LightZoneId id) const;

  void setLightLevel(LightZoneId zoneId, uint8_t percent);
  void setLightOn(LightZoneId zoneId);
  void setLightOff(LightZoneId zoneId);
  void fadeLight(LightZoneId zoneId, uint8_t targetPercent, uint32_t durationMs);
  void toggleLight(LightZoneId zoneId);
  void setBothLights(uint8_t percent);
  void allLightsOff();
  void allLightsOn();

  const LightSettings& settings() const;

  bool applySettingsJson(JsonObject obj);
  void settingsToJson(JsonObject obj) const;

  bool schedulesFromJson(JsonArray arr);
  void schedulesToJson(JsonArray arr) const;

  RoutineId routineFromName(const char* name) const;
  bool routinesFromJson(JsonArray arr);
  void startRoutine(RoutineId id);
  void stopRoutine();
  void toggleRoutine(RoutineId id);
  void cancelRoutine();
  bool routineActive() const;
  RoutineId routineActiveId() const;
  const char* routineStateName() const;
  const char* routineStepName() const;
  void routinesToJson(JsonDocument& doc, bool includeRoutineConfig = true) const;

  void setAlternateEnabled(bool enabled);
  bool alternateEnabled() const;
  void setAlternatePeriodMs(uint32_t ms);
  void setAlternateLevel(uint8_t percent);
  uint32_t alternatePeriodMs() const;
  uint8_t alternateLevel() const;
  void alternateToJson(JsonObject obj) const;

  void cancelAutomationForManualControl();
  void cancelAutomationIfFlashing();

 private:
  struct Impl;
  Impl* impl_;
  LightZone zones_[ZONE_COUNT];
};
