#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "LightController.h"
#include "AlternateMode.h"

enum ScheduleZone : uint8_t {
  SCHED_ZONE_CUNA = 0,
  SCHED_ZONE_SETUP = 1,
  SCHED_ZONE_BOTH = 2
};

enum ScheduleAction : uint8_t {
  SCHED_OFF = 0,
  SCHED_ON = 1,
  SCHED_LEVEL = 2
};

struct ScheduleEntry {
  bool enabled = false;
  uint8_t hour = 0;
  uint8_t minute = 0;
  ScheduleZone zone = SCHED_ZONE_BOTH;
  ScheduleAction action = SCHED_OFF;
  uint8_t level = 50;
};

class ScheduleManager {
 public:
  void begin();
  void startNtp();

  bool timeSynced() const { return timeSynced_; }
  String currentTimeText();

  size_t count() const { return count_; }
  const ScheduleEntry& entry(size_t i) const { return entries_[i]; }

  bool setEntries(const ScheduleEntry* src, size_t n);
  void clearAll();

  void update(LightController& lights, AlternateMode& alternate);

  void toJson(JsonArray arr) const;
  bool fromJson(JsonArray arr);

 private:
  void loadFromNvs();
  void saveToNvs();
  void applyEntry(const ScheduleEntry& e, LightController& lights, AlternateMode& alternate);

  ScheduleEntry entries_[MAX_SCHEDULES];
  size_t count_ = 0;

  Preferences prefs_;
  bool timeSynced_ = false;
  int lastCheckedMinuteOfDay_ = -1;
};
