#include "LightZone.h"
#include "LightSettings.h"
#include <cmath>

namespace {
uint8_t clampPercent(int value) {
  if (value < 0) return 0;
  if (value > 100) return 100;
  return static_cast<uint8_t>(value);
}

float clamp01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

float linearToPwmCurve(float linear) {
  linear = clamp01(linear);
  if (linear <= 0.0f) return 0.0f;
  return powf(linear, appSettings().get().fadeGamma);
}
}  // namespace

LightZone::LightZone(LightZoneId id, uint8_t gpio, uint8_t ledcChannel)
    : id_(id), gpio_(gpio), ledcChannel_(ledcChannel) {}

float LightZone::percentToBrightness(uint8_t percent) {
  return clamp01(static_cast<float>(percent) / 100.0f);
}

uint8_t LightZone::currentLevel() const {
  return clampPercent(static_cast<int>(brightness_ * 100.0f + 0.5f));
}

uint8_t LightZone::targetLevel() const {
  if (fadeActive_) {
    return clampPercent(static_cast<int>(fadeTargetBright_ * 100.0f + 0.5f));
  }
  return currentLevel();
}

const char* LightZone::phaseName() const {
  if (fadeActive_) {
    return fadeTargetBright_ <= 0.002f ? "turning_off" : "turning_on";
  }
  return isOn() ? "on" : "off";
}

void LightZone::begin() {
  ledcAttach(gpio_, PWM_FREQUENCY, PWM_RESOLUTION);
  applyPwmFromBrightness(0.0f);
}

void LightZone::update() { updateFade(); }

void LightZone::setLightLevel(uint8_t percent) {
  fadeActive_ = false;
  brightness_ = percentToBrightness(clampPercent(percent));
  applyPwmFromBrightness(brightness_);
}

void LightZone::setLightOn() {
  const LightSettings& cfg = appSettings().get();
  fadeLight(cfg.defaultOnLevel, cfg.fadeOnMs);
}

void LightZone::setLightOff() {
  fadeLight(0, appSettings().get().fadeOffMs);
}

void LightZone::fadeLight(uint8_t targetPercent, uint32_t durationMs) {
  targetPercent = clampPercent(targetPercent);
  if (durationMs == 0) {
    setLightLevel(targetPercent);
    return;
  }

  fadeActive_ = true;
  fadeStartMs_ = millis();
  fadeDurationMs_ = durationMs;
  fadeStartBright_ = brightness_;
  fadeTargetBright_ = percentToBrightness(targetPercent);
}

void LightZone::toggleLight() {
  const bool goingOrOn =
      fadeActive_ ? (fadeTargetBright_ > 0.002f) : (brightness_ > 0.002f);
  if (goingOrOn) {
    setLightOff();
  } else {
    setLightOn();
  }
}

void LightZone::applyPwmFromBrightness(float linear0to1) {
  const float curved = linearToPwmCurve(linear0to1);
  const uint32_t duty =
      static_cast<uint32_t>(curved * static_cast<float>(PWM_MAX_DUTY) + 0.5f);
  ledcWrite(gpio_, duty);
}

void LightZone::updateFade() {
  if (!fadeActive_) return;

  const uint32_t elapsed = millis() - fadeStartMs_;
  if (elapsed >= fadeDurationMs_) {
    brightness_ = fadeTargetBright_;
    applyPwmFromBrightness(brightness_);
    fadeActive_ = false;
    return;
  }

  float t = static_cast<float>(elapsed) / static_cast<float>(fadeDurationMs_);
  if (t > 1.0f) t = 1.0f;
  const float eased = t * t * (3.0f - 2.0f * t);

  brightness_ =
      fadeStartBright_ + (fadeTargetBright_ - fadeStartBright_) * eased;
  applyPwmFromBrightness(brightness_);
}
