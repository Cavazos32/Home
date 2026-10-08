#pragma once

#include "LightZone.h"

class LightController {
 public:
  LightController();

  void begin();
  void update();

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

 private:
  LightZone zones_[ZONE_COUNT];
};
