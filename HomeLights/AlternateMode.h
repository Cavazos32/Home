#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "LightController.h"

class AlternateMode {
 public:
  void begin();

  void setEnabled(bool enabled, LightController& lights);
  bool isEnabled() const { return enabled_; }

  void setPeriodMs(uint32_t ms);
  void setLevel(uint8_t percent);

  uint32_t periodMs() const { return periodMs_; }
  uint8_t level() const { return level_; }

  void update(LightController& lights);

 private:
  void applyPhase(LightController& lights);
  void saveConfig();

  Preferences prefs_;
  bool enabled_ = false;
  bool phaseSetupOn_ = false;
  uint32_t periodMs_ = 700;
  uint8_t level_ = 80;
  uint32_t lastToggleMs_ = 0;
};
