#include "AlternateMode.h"
#include <Preferences.h>

namespace {
const char* PREFS_NS = "homelights";
}  // namespace

void AlternateMode::begin() {
  prefs_.begin(PREFS_NS, false);
  periodMs_ = prefs_.getUInt("altPeriod", 700);
  level_ = prefs_.getUChar("altLevel", 80);
  if (periodMs_ < 200) periodMs_ = 200;
  if (level_ > 100) level_ = 100;
  enabled_ = false;
}

void AlternateMode::saveConfig() {
  prefs_.putUInt("altPeriod", periodMs_);
  prefs_.putUChar("altLevel", level_);
}

void AlternateMode::setEnabled(bool enabled, LightController& lights) {
  enabled_ = enabled;
  if (enabled_) {
    lastToggleMs_ = millis();
    phaseSetupOn_ = false;
    applyPhase(lights);
  }
  // Al desactivar no se restaura el estado previo: las zonas quedan en la
  // última fase aplicada por el modo alternado (comportamiento intencional).
}

void AlternateMode::setPeriodMs(uint32_t ms) {
  periodMs_ = ms < 200 ? 200 : ms;
  saveConfig();
}

void AlternateMode::setLevel(uint8_t percent) {
  level_ = percent > 100 ? 100 : percent;
  saveConfig();
}

void AlternateMode::applyPhase(LightController& lights) {
  if (phaseSetupOn_) {
    lights.setLightLevel(ZONE_CUNA, 0);
    lights.setLightLevel(ZONE_SETUP, level_);
  } else {
    lights.setLightLevel(ZONE_CUNA, level_);
    lights.setLightLevel(ZONE_SETUP, 0);
  }
}

void AlternateMode::update(LightController& lights) {
  if (!enabled_) return;

  const uint32_t now = millis();
  if (now - lastToggleMs_ < periodMs_) return;

  lastToggleMs_ = now;
  phaseSetupOn_ = !phaseSetupOn_;
  applyPhase(lights);
}
