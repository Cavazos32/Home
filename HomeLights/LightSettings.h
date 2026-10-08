#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"

struct LightSettings {
  uint32_t fadeOnMs = FADE_ON_MS;
  uint32_t fadeOffMs = FADE_OFF_MS;
  float fadeGamma = FADE_GAMMA;
  uint8_t defaultOnLevel = DEFAULT_ON_LEVEL;
};

class LightSettingsStore {
 public:
  void begin();

  const LightSettings& get() const { return data_; }
  LightSettings& mutableGet() { return data_; }

  bool applyAndSave(const LightSettings& next);
  void toJson(JsonObject obj) const;
  bool fromJson(JsonObject obj);

 private:
  void loadFromNvs();
  void saveToNvs();
  void clampInPlace();

  LightSettings data_;
  Preferences prefs_;
};

LightSettingsStore& appSettings();
