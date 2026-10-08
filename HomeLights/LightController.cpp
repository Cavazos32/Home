#include "LightController.h"

LightController::LightController()
    : zones_{LightZone(ZONE_CUNA, GPIO_LIGHT_CUNA, LEDC_CHANNEL_CUNA),
             LightZone(ZONE_SETUP, GPIO_LIGHT_SETUP, LEDC_CHANNEL_SETUP)} {}

void LightController::begin() {
  for (auto& z : zones_) {
    z.begin();
  }
}

void LightController::update() {
  for (auto& z : zones_) {
    z.update();
  }
}

LightZone& LightController::zone(LightZoneId id) { return zones_[id]; }

const LightZone& LightController::zone(LightZoneId id) const { return zones_[id]; }

void LightController::setLightLevel(LightZoneId zoneId, uint8_t percent) {
  zones_[zoneId].setLightLevel(percent);
}

void LightController::setLightOn(LightZoneId zoneId) {
  zones_[zoneId].setLightOn();
}

void LightController::setLightOff(LightZoneId zoneId) {
  zones_[zoneId].setLightOff();
}

void LightController::fadeLight(LightZoneId zoneId, uint8_t targetPercent,
                                uint32_t durationMs) {
  zones_[zoneId].fadeLight(targetPercent, durationMs);
}

void LightController::toggleLight(LightZoneId zoneId) {
  zones_[zoneId].toggleLight();
}

void LightController::setBothLights(uint8_t percent) {
  setLightLevel(ZONE_CUNA, percent);
  setLightLevel(ZONE_SETUP, percent);
}

void LightController::allLightsOff() {
  setLightOff(ZONE_CUNA);
  setLightOff(ZONE_SETUP);
}

void LightController::allLightsOn() {
  setLightOn(ZONE_CUNA);
  setLightOn(ZONE_SETUP);
}
