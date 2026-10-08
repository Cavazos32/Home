#include "LightSettings.h"

namespace {
LightSettingsStore g_store;
const char* PREFS_NS = "homelights";
}  // namespace

LightSettingsStore& appSettings() { return g_store; }

void LightSettingsStore::begin() {
  prefs_.begin(PREFS_NS, false);
  loadFromNvs();
}

void LightSettingsStore::clampInPlace() {
  if (data_.fadeOnMs < 200) data_.fadeOnMs = 200;
  if (data_.fadeOnMs > 15000) data_.fadeOnMs = 15000;
  if (data_.fadeOffMs < 200) data_.fadeOffMs = 200;
  if (data_.fadeOffMs > 15000) data_.fadeOffMs = 15000;
  if (data_.fadeGamma < 1.0f) data_.fadeGamma = 1.0f;
  if (data_.fadeGamma > 4.0f) data_.fadeGamma = 4.0f;
  if (data_.defaultOnLevel < 1) data_.defaultOnLevel = 1;
  if (data_.defaultOnLevel > 100) data_.defaultOnLevel = 100;
}

void LightSettingsStore::loadFromNvs() {
  data_.fadeOnMs = prefs_.getUInt("fadeOn", FADE_ON_MS);
  data_.fadeOffMs = prefs_.getUInt("fadeOff", FADE_OFF_MS);
  data_.fadeGamma = prefs_.getFloat("gamma", FADE_GAMMA);
  data_.defaultOnLevel =
      prefs_.getUChar("defOn", static_cast<uint8_t>(DEFAULT_ON_LEVEL));
  clampInPlace();
}

void LightSettingsStore::saveToNvs() {
  prefs_.putUInt("fadeOn", data_.fadeOnMs);
  prefs_.putUInt("fadeOff", data_.fadeOffMs);
  prefs_.putFloat("gamma", data_.fadeGamma);
  prefs_.putUChar("defOn", data_.defaultOnLevel);
}

bool LightSettingsStore::applyAndSave(const LightSettings& next) {
  data_ = next;
  clampInPlace();
  saveToNvs();
  return true;
}

void LightSettingsStore::toJson(JsonObject obj) const {
  obj["fadeOnMs"] = data_.fadeOnMs;
  obj["fadeOffMs"] = data_.fadeOffMs;
  obj["fadeGamma"] = data_.fadeGamma;
  obj["defaultOnLevel"] = data_.defaultOnLevel;
}

bool LightSettingsStore::fromJson(JsonObject obj) {
  LightSettings next = data_;

  if (!obj["fadeOnMs"].isNull()) {
    const int value = obj["fadeOnMs"].as<int>();
    if (value < 200 || value > 15000) return false;
    next.fadeOnMs = static_cast<uint32_t>(value);
  }
  if (!obj["fadeOffMs"].isNull()) {
    const int value = obj["fadeOffMs"].as<int>();
    if (value < 200 || value > 15000) return false;
    next.fadeOffMs = static_cast<uint32_t>(value);
  }
  if (!obj["fadeGamma"].isNull()) {
    const float value = obj["fadeGamma"].as<float>();
    if (value < 1.0f || value > 4.0f) return false;
    next.fadeGamma = value;
  }
  if (!obj["defaultOnLevel"].isNull()) {
    const int value = obj["defaultOnLevel"].as<int>();
    if (value < 1 || value > 100) return false;
    next.defaultOnLevel = static_cast<uint8_t>(value);
  }

  return applyAndSave(next);
}
