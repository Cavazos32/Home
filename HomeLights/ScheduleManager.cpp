#include "ScheduleManager.h"
#include <ArduinoJson.h>
#include <time.h>

namespace {
const char* PREFS_NS = "homelights";
const char* PREFS_KEY = "schedules";

int minuteOfDay(const struct tm& ti) { return ti.tm_hour * 60 + ti.tm_min; }

int dayMinuteKey(const struct tm& ti) {
  return ti.tm_yday * 1440 + minuteOfDay(ti);
}
}  // namespace

void ScheduleManager::begin() {
  prefs_.begin(PREFS_NS, false);
  loadFromNvs();
}

void ScheduleManager::startNtp() {
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
}

String ScheduleManager::currentTimeText() {
  struct tm ti;
  if (!getLocalTime(&ti, 50)) {
    return timeSynced_ ? "Sincronizando…" : "Sin hora (NTP)";
  }
  timeSynced_ = true;
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
  return String(buf);
}

bool ScheduleManager::setEntries(const ScheduleEntry* src, size_t n) {
  if (n > MAX_SCHEDULES) return false;
  count_ = n;
  for (size_t i = 0; i < n; i++) {
    entries_[i] = src[i];
  }
  saveToNvs();
  return true;
}

void ScheduleManager::clearAll() {
  count_ = 0;
  saveToNvs();
}

void ScheduleManager::applyEntry(const ScheduleEntry& e, LightController& lights,
                                 AlternateMode& alternate) {
  alternate.setEnabled(false, lights);

  auto applyZone = [&](LightZoneId id) {
    switch (e.action) {
      case SCHED_OFF:
        lights.setLightOff(id);
        break;
      case SCHED_ON:
        lights.setLightOn(id);
        break;
      case SCHED_LEVEL:
        lights.setLightLevel(id, e.level);
        break;
    }
  };

  if (e.zone == SCHED_ZONE_BOTH) {
    if (e.action == SCHED_OFF) {
      lights.allLightsOff();
    } else if (e.action == SCHED_ON) {
      lights.allLightsOn();
    } else {
      lights.setBothLights(e.level);
    }
    return;
  }

  applyZone(e.zone == SCHED_ZONE_CUNA ? ZONE_CUNA : ZONE_SETUP);
}

void ScheduleManager::update(LightController& lights, AlternateMode& alternate) {
  struct tm ti;
  if (!getLocalTime(&ti, 20)) return;
  timeSynced_ = true;

  const int key = dayMinuteKey(ti);
  if (key == lastCheckedDayMinuteKey_) return;
  lastCheckedDayMinuteKey_ = key;

  for (size_t i = 0; i < count_; i++) {
    const ScheduleEntry& e = entries_[i];
    if (!e.enabled) continue;
    if (e.hour == static_cast<uint8_t>(ti.tm_hour) &&
        e.minute == static_cast<uint8_t>(ti.tm_min)) {
      applyEntry(e, lights, alternate);
    }
  }
}

void ScheduleManager::loadFromNvs() {
  count_ = 0;
  size_t len = prefs_.getBytesLength(PREFS_KEY);
  if (len == 0 || len % sizeof(ScheduleEntry) != 0) return;

  size_t n = len / sizeof(ScheduleEntry);
  if (n > MAX_SCHEDULES) n = MAX_SCHEDULES;

  prefs_.getBytes(PREFS_KEY, entries_, n * sizeof(ScheduleEntry));
  count_ = n;
}

void ScheduleManager::saveToNvs() {
  if (count_ == 0) {
    prefs_.remove(PREFS_KEY);
    return;
  }
  prefs_.putBytes(PREFS_KEY, entries_, count_ * sizeof(ScheduleEntry));
}

void ScheduleManager::toJson(JsonArray arr) const {
  for (size_t i = 0; i < count_; i++) {
    JsonObject o = arr.add<JsonObject>();
    const ScheduleEntry& e = entries_[i];
    o["enabled"] = e.enabled;
    o["hour"] = e.hour;
    o["minute"] = e.minute;
    o["zone"] = e.zone == SCHED_ZONE_CUNA   ? "cuna"
                : e.zone == SCHED_ZONE_SETUP ? "setup"
                                             : "both";
    o["action"] = e.action == SCHED_OFF ? "off" : e.action == SCHED_ON ? "on" : "level";
    o["level"] = e.level;
  }
}

bool ScheduleManager::fromJson(JsonArray arr) {
  ScheduleEntry tmp[MAX_SCHEDULES];
  size_t n = 0;

  for (JsonObject o : arr) {
    if (n >= MAX_SCHEDULES) return false;

    ScheduleEntry e;
    e.enabled = o["enabled"] | false;
    e.hour = o["hour"] | 0;
    e.minute = o["minute"] | 0;
    e.level = o["level"] | 50;

    const char* zone = o["zone"] | "both";
    if (strcmp(zone, "cuna") == 0) {
      e.zone = SCHED_ZONE_CUNA;
    } else if (strcmp(zone, "setup") == 0) {
      e.zone = SCHED_ZONE_SETUP;
    } else {
      e.zone = SCHED_ZONE_BOTH;
    }

    const char* action = o["action"] | "off";
    if (strcmp(action, "on") == 0) {
      e.action = SCHED_ON;
    } else if (strcmp(action, "level") == 0) {
      e.action = SCHED_LEVEL;
    } else {
      e.action = SCHED_OFF;
    }

    if (e.hour > 23 || e.minute > 59 || e.level > 100) return false;

    tmp[n++] = e;
  }

  return setEntries(tmp, n);
}
