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
    next.fadeOnMs = obj["fadeOnMs"].as<uint32_t>();
  }
  if (!obj["fadeOffMs"].isNull()) {
    next.fadeOffMs = obj["fadeOffMs"].as<uint32_t>();
  }
  if (!obj["fadeGamma"].isNull()) {
    next.fadeGamma = obj["fadeGamma"].as<float>();
  }
  if (!obj["defaultOnLevel"].isNull()) {
    next.defaultOnLevel = obj["defaultOnLevel"].as<uint8_t>();
  }

  return applyAndSave(next);
}
