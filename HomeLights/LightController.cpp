#include "LightController.h"

#include <Preferences.h>

#include <cmath>
#include <cstring>

namespace {
const char* kPrefsNs = "homelights";

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

float linearToPwmCurve(float linear, float gamma) {
  linear = clamp01(linear);
  if (linear <= 0.0f) return 0.0f;
  return powf(linear, gamma);
}

int minuteOfDay(const struct tm& ti) { return ti.tm_hour * 60 + ti.tm_min; }

int dayMinuteKey(const struct tm& ti) { return ti.tm_yday * 1440 + minuteOfDay(ti); }
}  // namespace

// --- LightZone ----------------------------------------------------------------

LightZone::LightZone(uint8_t gpio) : gpio_(gpio) {}

void LightZone::bindSettings(const LightSettings* settings) { settings_ = settings; }

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

float LightZone::transitionProgress() const {
  if (!fadeActive_) return 1.0f;
  const float span = fadeTargetBright_ - fadeStartBright_;
  if (fabsf(span) < 0.0005f) return 1.0f;
  return clamp01((brightness_ - fadeStartBright_) / span);
}

const char* LightZone::phaseName() const {
  if (fadeActive_) {
    return fadeTargetBright_ <= 0.002f ? "turning_off" : "turning_on";
  }
  return isOn() ? "on" : "off";
}

bool LightZone::isLit() const {
  if (fadeActive_) {
    return fadeTargetBright_ > 0.002f || brightness_ > 0.002f;
  }
  return brightness_ > 0.002f;
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
  if (!settings_) return;
  fadeLight(settings_->defaultOnLevel, settings_->fadeOnMs);
}

void LightZone::setLightOff() {
  if (!settings_) return;
  fadeLight(0, settings_->fadeOffMs);
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
  const float gamma = settings_ ? settings_->fadeGamma : FADE_GAMMA;
  const float curved = linearToPwmCurve(linear0to1, gamma);
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
  brightness_ = fadeStartBright_ + (fadeTargetBright_ - fadeStartBright_) * eased;
  applyPwmFromBrightness(brightness_);
}

// --- Impl interno (ajustes, rutinas, horarios, alternado) ---------------------

struct LightController::Impl {
  Preferences prefs;

  class SettingsStore {
   public:
    void begin(Preferences& prefs) {
      prefs_ = &prefs;
      loadFromNvs();
    }

    const LightSettings& get() const { return data_; }

    bool applyAndSave(const LightSettings& next) {
      data_ = next;
      clampInPlace();
      saveToNvs();
      return true;
    }

    void toJson(JsonObject obj) const {
      obj["fadeOnMs"] = data_.fadeOnMs;
      obj["fadeOffMs"] = data_.fadeOffMs;
      obj["fadeGamma"] = data_.fadeGamma;
      obj["defaultOnLevel"] = data_.defaultOnLevel;
    }

    bool fromJson(JsonObject obj) {
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

   private:
    void clampInPlace() {
      if (data_.fadeOnMs < 200) data_.fadeOnMs = 200;
      if (data_.fadeOnMs > 15000) data_.fadeOnMs = 15000;
      if (data_.fadeOffMs < 200) data_.fadeOffMs = 200;
      if (data_.fadeOffMs > 15000) data_.fadeOffMs = 15000;
      if (data_.fadeGamma < 1.0f) data_.fadeGamma = 1.0f;
      if (data_.fadeGamma > 4.0f) data_.fadeGamma = 4.0f;
      if (data_.defaultOnLevel < 1) data_.defaultOnLevel = 1;
      if (data_.defaultOnLevel > 100) data_.defaultOnLevel = 100;
    }

    void loadFromNvs() {
      if (!prefs_) return;
      data_.fadeOnMs = prefs_->getUInt("fadeOn", FADE_ON_MS);
      data_.fadeOffMs = prefs_->getUInt("fadeOff", FADE_OFF_MS);
      data_.fadeGamma = prefs_->getFloat("gamma", FADE_GAMMA);
      data_.defaultOnLevel =
          prefs_->getUChar("defOn", static_cast<uint8_t>(DEFAULT_ON_LEVEL));
      clampInPlace();
    }

    void saveToNvs() {
      if (!prefs_) return;
      prefs_->putUInt("fadeOn", data_.fadeOnMs);
      prefs_->putUInt("fadeOff", data_.fadeOffMs);
      prefs_->putFloat("gamma", data_.fadeGamma);
      prefs_->putUChar("defOn", data_.defaultOnLevel);
    }

    LightSettings data_;
    Preferences* prefs_ = nullptr;
  };

  class AlternateMode {
   public:
    void begin(Preferences& prefs) {
      prefs_ = &prefs;
      periodMs_ = prefs_->getUInt("altPeriod", 700);
      level_ = prefs_->getUChar("altLevel", 80);
      if (periodMs_ > 5000) periodMs_ = 5000;
      if (level_ > 100) level_ = 100;
      enabled_ = false;
    }

    void setEnabled(bool enabled, LightController& lights) {
      enabled_ = enabled;
      if (enabled_) {
        lastToggleMs_ = millis();
        phaseSetupOn_ = false;
        applyPhase(lights);
      }
    }

    bool isEnabled() const { return enabled_; }
    void setPeriodMs(uint32_t ms) {
      if (ms > 5000) ms = 5000;
      periodMs_ = ms;
      saveConfig();
    }
    void setLevel(uint8_t percent) {
      if (percent > 100) percent = 100;
      level_ = percent;
      saveConfig();
    }
    uint32_t periodMs() const { return periodMs_; }
    uint8_t level() const { return level_; }

    void update(LightController& lights) {
      if (!enabled_) return;
      const uint32_t now = millis();
      if (now - lastToggleMs_ < periodMs_) return;
      lastToggleMs_ = now;
      phaseSetupOn_ = !phaseSetupOn_;
      applyPhase(lights);
    }

   private:
    void applyPhase(LightController& lights) {
      if (phaseSetupOn_) {
        lights.setLightLevel(ZONE_CUNA, 0);
        lights.setLightLevel(ZONE_SETUP, level_);
      } else {
        lights.setLightLevel(ZONE_CUNA, level_);
        lights.setLightLevel(ZONE_SETUP, 0);
      }
    }

    void saveConfig() {
      if (!prefs_) return;
      prefs_->putUInt("altPeriod", periodMs_);
      prefs_->putUChar("altLevel", level_);
    }

    Preferences* prefs_ = nullptr;
    bool enabled_ = false;
    bool phaseSetupOn_ = false;
    uint32_t periodMs_ = 700;
    uint8_t level_ = 80;
    uint32_t lastToggleMs_ = 0;
  };

  class RoutineManager {
   public:
    void begin(Preferences& prefs) {
      prefs_ = &prefs;
      loadFromNvs();
    }

    RoutineId idFromName(const char* name) const {
      if (name == nullptr) return ROUTINE_NONE;
      for (uint8_t i = 1; i < defCount_; ++i) {
        if (strcmp(name, defs_[i].id) == 0) return static_cast<RoutineId>(i);
      }
      return ROUTINE_NONE;
    }

    void start(RoutineId id, LightController& lights) {
      if (id == ROUTINE_NONE || id >= defCount_) {
        cancel();
        return;
      }
      active_ = id;
      step_ = 0;
      completed_ = false;
      waitingDelay_ = false;
      stepZone_ = defs_[id].steps[0].zone;
      if (settings_) update(lights, *settings_);
    }

    void stop(LightController& lights) {
      cancel();
      for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
        const LightZoneId id = static_cast<LightZoneId>(i);
        if (!headingOff(lights.zone(id))) lights.setLightOff(id);
      }
    }

    void toggle(RoutineId id, LightController& lights) {
      if (id != ROUTINE_NONE && active_ == id) {
        stop(lights);
      } else {
        start(id, lights);
      }
    }

    void cancel() {
      active_ = ROUTINE_NONE;
      step_ = 0;
      completed_ = false;
      waitingDelay_ = false;
    }

    void update(LightController& lights, const LightSettings& settings) {
      if (!running()) return;
      const RoutineDef& def = defs_[active_];
      const float advanceAt = def.nextStepAt > 0.01f ? def.nextStepAt : ROUTINE_NEXT_STEP_AT;

      if (waitingDelay_) {
        if (static_cast<int32_t>(millis() - stepWaitUntilMs_) < 0) return;
        waitingDelay_ = false;
      }

      while (step_ < def.stepCount) {
        const RoutineStep& s = def.steps[step_];
        const LightZone& z = lights.zone(s.zone);
        if (!headingTo(z, s.level)) {
          lights.fadeLight(s.zone, s.level, settings.fadeOnMs);
        }
        stepZone_ = s.zone;
        if (z.transitionProgress() < advanceAt) return;
        ++step_;
        if (step_ < def.stepCount && def.steps[step_].delayMs > 0) {
          stepWaitUntilMs_ = millis() + def.steps[step_].delayMs;
          waitingDelay_ = true;
          return;
        }
      }

      for (int i = static_cast<int>(def.stepCount) - 1; i >= 0; --i) {
        if (lights.zone(def.steps[i].zone).isTransitioning()) {
          stepZone_ = def.steps[i].zone;
          return;
        }
      }
      completed_ = true;
    }

    RoutineId active() const { return active_; }
    bool isActive() const { return active_ != ROUTINE_NONE; }
    bool running() const { return isActive() && !completed_; }

    const char* stateName() const {
      if (!isActive()) return "idle";
      return completed_ ? "completed" : "running";
    }

    const char* stepName() const { return running() ? zoneName(stepZone_) : "idle"; }

    void summaryJson(JsonObject obj) const {
      if (isActive()) {
        obj["active"] = defs_[active_].id;
      } else {
        obj["active"] = "";
      }
      obj["state"] = stateName();
      obj["step"] = stepName();
    }

    void listJson(JsonArray arr) const {
      for (uint8_t i = 1; i < defCount_; ++i) {
        appendDefJson(arr, defs_[i], active_ == i);
      }
    }

    void listRuntimeJson(JsonArray arr) const {
      for (uint8_t i = 1; i < defCount_; ++i) {
        const RoutineDef& def = defs_[i];
        const bool mine = active_ == i;
        JsonObject o = arr.add<JsonObject>();
        o["id"] = def.id;
        o["label"] = def.label;
        o["active"] = mine;
        o["state"] = mine ? stateName() : "idle";
        o["step"] = mine ? stepName() : "idle";
      }
    }

    void legacyGroupJson(JsonObject obj) const {
      const RoutineDef& hab = defs_[ROUTINE_HABITACION];
      const bool mine = active_ == ROUTINE_HABITACION;
      obj["id"] = hab.id;
      obj["active"] = mine;
      obj["running"] = mine && running();
      obj["step"] = mine ? stepName() : "idle";
      int cunaLevel = GROUP_CUNA_LEVEL;
      int setupLevel = GROUP_SETUP_LEVEL;
      for (uint8_t s = 0; s < hab.stepCount; ++s) {
        if (hab.steps[s].zone == ZONE_CUNA) cunaLevel = hab.steps[s].level;
        if (hab.steps[s].zone == ZONE_SETUP) setupLevel = hab.steps[s].level;
      }
      obj["cunaLevel"] = cunaLevel;
      obj["setupLevel"] = setupLevel;
    }

    bool fromJson(JsonArray arr) {
      RoutineDef tmp[kMaxDefs];
      uint8_t n = 0;

      for (JsonObject o : arr) {
        if (n >= kMaxDefs) return false;
        RoutineDef& def = tmp[n++];
        def = RoutineDef();

        const char* id = o["id"] | "";
        const char* label = o["label"] | id;
        if (!validSlug(id)) return false;
        strncpy(def.id, id, sizeof(def.id) - 1);
        strncpy(def.label, label, sizeof(def.label) - 1);

        const float nextAt = o["nextStepAt"] | ROUTINE_NEXT_STEP_AT;
        if (nextAt < 0.05f || nextAt > 1.0f) return false;
        def.nextStepAt = nextAt;

        JsonArray steps = o["steps"].as<JsonArray>();
        if (steps.isNull() || steps.size() == 0 || steps.size() > MAX_ROUTINE_STEPS) {
          return false;
        }
        def.stepCount = static_cast<uint8_t>(steps.size());
        uint8_t si = 0;
        for (JsonObject st : steps) {
          const char* zone = st["zone"] | "";
          const int level = st["level"] | -1;
          const int delayMs = st["delayMs"] | 0;
          LightZoneId zid;
          if (strcmp(zone, "cuna") == 0) {
            zid = ZONE_CUNA;
          } else if (strcmp(zone, "setup") == 0) {
            zid = ZONE_SETUP;
          } else {
            return false;
          }
          if (level < 0 || level > 100 || delayMs < 0 || delayMs > 600000) return false;
          def.steps[si].zone = zid;
          def.steps[si].level = static_cast<uint8_t>(level);
          def.steps[si].delayMs = static_cast<uint16_t>(delayMs);
          ++si;
        }
      }

      if (n == 0) return false;
      for (uint8_t a = 0; a < n; ++a) {
        for (uint8_t b = a + 1; b < n; ++b) {
          if (strcmp(tmp[a].id, tmp[b].id) == 0) return false;
        }
      }

      char activeId[sizeof(RoutineDef::id)] = {};
      if (isActive()) strncpy(activeId, defs_[active_].id, sizeof(activeId) - 1);
      if (!applyDefs(tmp, n)) return false;
      const RoutineId stillActive = activeId[0] ? idFromName(activeId) : ROUTINE_NONE;
      if (stillActive == ROUTINE_NONE) {
        cancel();
      } else {
        active_ = stillActive;
      }
      saveToNvs();
      return true;
    }

    void bindSettings(const LightSettings* s) { settings_ = s; }

   private:
    struct RoutineStep {
      LightZoneId zone = ZONE_CUNA;
      uint8_t level = 0;
      uint16_t delayMs = 0;
    };

    struct RoutineDef {
      char id[16] = {};
      char label[32] = {};
      uint8_t stepCount = 0;
      float nextStepAt = ROUTINE_NEXT_STEP_AT;
      RoutineStep steps[MAX_ROUTINE_STEPS];
    };

    static const char* zoneName(LightZoneId id) {
      return id == ZONE_CUNA ? "cuna" : "setup";
    }

    static bool validSlug(const char* id) {
      if (id == nullptr || id[0] == '\0') return false;
      if (id[0] < 'a' || id[0] > 'z') return false;
      for (const char* p = id; *p; ++p) {
        const char c = *p;
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') continue;
        return false;
      }
      return strlen(id) < sizeof(RoutineDef::id);
    }

    static bool nearLevel(int a, int b) {
      const int d = a - b;
      return d <= 1 && d >= -1;
    }

    static bool headingTo(const LightZone& z, uint8_t level) {
      return nearLevel(z.isTransitioning() ? z.targetLevel() : z.currentLevel(), level);
    }

    static bool headingOff(const LightZone& z) {
      return z.isTransitioning() ? z.targetLevel() == 0 : !z.isOn();
    }

    void appendDefJson(JsonArray arr, const RoutineDef& def, bool mine) const {
      JsonObject o = arr.add<JsonObject>();
      o["id"] = def.id;
      o["label"] = def.label;
      o["active"] = mine;
      o["state"] = mine ? stateName() : "idle";
      o["step"] = mine ? stepName() : "idle";
      o["nextStepAt"] = def.nextStepAt;
      JsonArray steps = o["steps"].to<JsonArray>();
      for (uint8_t s = 0; s < def.stepCount; ++s) {
        JsonObject step = steps.add<JsonObject>();
        step["zone"] = zoneName(def.steps[s].zone);
        step["level"] = def.steps[s].level;
        step["delayMs"] = def.steps[s].delayMs;
      }
    }

    // defs_[0] es ROUTINE_NONE; las definiciones reales ocupan defs_[1..kMaxDefs].
    static const uint8_t kMaxDefs = MAX_ROUTINE_GROUPS - 1;

    static RoutineDef habitacionDefault() {
      RoutineDef d;
      strncpy(d.id, "habitacion", sizeof(d.id) - 1);
      strncpy(d.label, "Habitación", sizeof(d.label) - 1);
      d.stepCount = 2;
      d.steps[0] = {ZONE_CUNA, GROUP_CUNA_LEVEL, 0};
      d.steps[1] = {ZONE_SETUP, GROUP_SETUP_LEVEL, 0};
      return d;
    }

    static RoutineDef babyDefault() {
      RoutineDef d;
      strncpy(d.id, "baby", sizeof(d.id) - 1);
      strncpy(d.label, "Baby", sizeof(d.label) - 1);
      d.stepCount = 2;
      d.steps[0] = {ZONE_SETUP, BABY_SETUP_LEVEL, 0};
      d.steps[1] = {ZONE_CUNA, BABY_CUNA_LEVEL, 0};
      return d;
    }

    // Habitación y Baby ocupan siempre ROUTINE_HABITACION y ROUTINE_BABY (atajos y API antigua);
    // si faltan en la lista se restauran con sus valores por defecto.
    bool applyDefs(const RoutineDef* list, uint8_t n) {
      RoutineDef ordered[kMaxDefs];
      int hab = -1;
      int baby = -1;
      for (uint8_t i = 0; i < n; ++i) {
        if (strcmp(list[i].id, "habitacion") == 0) hab = i;
        if (strcmp(list[i].id, "baby") == 0) baby = i;
      }
      uint8_t count = 0;
      ordered[count++] = hab >= 0 ? list[hab] : habitacionDefault();
      ordered[count++] = baby >= 0 ? list[baby] : babyDefault();
      for (uint8_t i = 0; i < n; ++i) {
        if (static_cast<int>(i) == hab || static_cast<int>(i) == baby) continue;
        if (count >= kMaxDefs) return false;
        ordered[count++] = list[i];
      }
      for (uint8_t i = 0; i < count; ++i) defs_[i + 1] = ordered[i];
      defCount_ = static_cast<uint8_t>(count + 1);
      return true;
    }

    void seedDefaults() {
      const RoutineDef none[1] = {};
      applyDefs(none, 0);
    }

    void loadFromNvs() {
      seedDefaults();
      if (!prefs_) return;
      size_t len = prefs_->getBytesLength("routineDefs");
      if (len == 0 || len % sizeof(RoutineDef) != 0) return;
      size_t n = len / sizeof(RoutineDef);
      if (n == 0 || n > kMaxDefs) return;
      RoutineDef loaded[kMaxDefs];
      prefs_->getBytes("routineDefs", loaded, len);
      for (size_t i = 0; i < n; ++i) {
        loaded[i].id[sizeof(loaded[i].id) - 1] = '\0';
        loaded[i].label[sizeof(loaded[i].label) - 1] = '\0';
        if (!validSlug(loaded[i].id) || loaded[i].stepCount == 0 ||
            loaded[i].stepCount > MAX_ROUTINE_STEPS) {
          return;
        }
        for (uint8_t s = 0; s < loaded[i].stepCount; ++s) {
          if (loaded[i].steps[s].zone > ZONE_SETUP || loaded[i].steps[s].level > 100) {
            return;
          }
        }
      }
      if (!applyDefs(loaded, static_cast<uint8_t>(n))) seedDefaults();
    }

    void saveToNvs() {
      if (!prefs_ || defCount_ <= 1) return;
      prefs_->putBytes("routineDefs", &defs_[1], (defCount_ - 1) * sizeof(RoutineDef));
    }

    RoutineDef defs_[MAX_ROUTINE_GROUPS];
    uint8_t defCount_ = 3;
    RoutineId active_ = ROUTINE_NONE;
    uint8_t step_ = 0;
    bool completed_ = false;
    bool waitingDelay_ = false;
    uint32_t stepWaitUntilMs_ = 0;
    LightZoneId stepZone_ = ZONE_CUNA;
    const LightSettings* settings_ = nullptr;
    Preferences* prefs_ = nullptr;
  };

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
    void begin(Preferences& prefs) {
      prefs_ = &prefs;
      loadFromNvs();
    }

    void startNtp() {
      configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
    }

    bool timeSynced() const { return timeSynced_; }

    String currentTimeText() {
      struct tm ti;
      if (!getLocalTime(&ti, 50)) {
        return timeSynced_ ? "Sincronizando…" : "Sin hora (NTP)";
      }
      timeSynced_ = true;
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
      return String(buf);
    }

    bool update(LightController& lights, AlternateMode& alternate) {
      struct tm ti;
      if (!getLocalTime(&ti, 20)) return false;
      timeSynced_ = true;

      const int key = dayMinuteKey(ti);
      if (key == lastCheckedDayMinuteKey_) return false;
      lastCheckedDayMinuteKey_ = key;

      bool applied = false;
      for (size_t i = 0; i < count_; i++) {
        const ScheduleEntry& e = entries_[i];
        if (!e.enabled) continue;
        if (e.hour == static_cast<uint8_t>(ti.tm_hour) &&
            e.minute == static_cast<uint8_t>(ti.tm_min)) {
          applyEntry(e, lights, alternate);
          applied = true;
        }
      }
      return applied;
    }

    void toJson(JsonArray arr) const {
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

    bool fromJson(JsonArray arr) {
      ScheduleEntry tmp[MAX_SCHEDULES];
      size_t n = 0;

      for (JsonObject o : arr) {
        if (n >= MAX_SCHEDULES) return false;

        ScheduleEntry e;
        e.enabled = o["enabled"] | false;
        const int hour = o["hour"] | 0;
        const int minute = o["minute"] | 0;
        const int level = o["level"] | 50;
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || level < 0 || level > 100) {
          return false;
        }
        e.hour = static_cast<uint8_t>(hour);
        e.minute = static_cast<uint8_t>(minute);
        e.level = static_cast<uint8_t>(level);

        const char* zone = o["zone"] | "both";
        if (strcmp(zone, "cuna") == 0) {
          e.zone = SCHED_ZONE_CUNA;
        } else if (strcmp(zone, "setup") == 0) {
          e.zone = SCHED_ZONE_SETUP;
        } else if (strcmp(zone, "both") == 0) {
          e.zone = SCHED_ZONE_BOTH;
        } else {
          return false;
        }

        const char* action = o["action"] | "off";
        if (strcmp(action, "on") == 0) {
          e.action = SCHED_ON;
        } else if (strcmp(action, "level") == 0) {
          e.action = SCHED_LEVEL;
        } else if (strcmp(action, "off") == 0) {
          e.action = SCHED_OFF;
        } else {
          return false;
        }

        tmp[n++] = e;
      }

      return setEntries(tmp, n);
    }

   private:
    bool setEntries(const ScheduleEntry* src, size_t n) {
      if (n > MAX_SCHEDULES) return false;
      count_ = n;
      for (size_t i = 0; i < n; i++) {
        entries_[i] = src[i];
      }
      saveToNvs();
      return true;
    }

    void applyEntry(const ScheduleEntry& e, LightController& lights, AlternateMode& alternate) {
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

    void loadFromNvs() {
      count_ = 0;
      if (!prefs_) return;
      size_t len = prefs_->getBytesLength("schedules");
      if (len == 0 || len % sizeof(ScheduleEntry) != 0) return;
      size_t n = len / sizeof(ScheduleEntry);
      if (n > MAX_SCHEDULES) n = MAX_SCHEDULES;
      prefs_->getBytes("schedules", entries_, n * sizeof(ScheduleEntry));
      count_ = n;
    }

    void saveToNvs() {
      if (!prefs_) return;
      if (count_ == 0) {
        prefs_->remove("schedules");
        return;
      }
      prefs_->putBytes("schedules", entries_, count_ * sizeof(ScheduleEntry));
    }

    ScheduleEntry entries_[MAX_SCHEDULES];
    size_t count_ = 0;
    Preferences* prefs_ = nullptr;
    bool timeSynced_ = false;
    int lastCheckedDayMinuteKey_ = -1;
  };

  SettingsStore settings;
  AlternateMode alternate;
  RoutineManager routines;
  ScheduleManager schedules;
};

// --- LightController ----------------------------------------------------------

LightController::LightController()
    : impl_(new Impl()), zones_{LightZone(GPIO_LIGHT_CUNA), LightZone(GPIO_LIGHT_SETUP)} {}

LightController::~LightController() { delete impl_; }

void LightController::begin() {
  impl_->prefs.begin(kPrefsNs, false);
  impl_->settings.begin(impl_->prefs);
  const LightSettings* cfg = &impl_->settings.get();
  for (auto& z : zones_) {
    z.bindSettings(cfg);
  }
  impl_->routines.bindSettings(cfg);
  impl_->routines.begin(impl_->prefs);
  impl_->alternate.begin(impl_->prefs);
  impl_->schedules.begin(impl_->prefs);
  for (auto& z : zones_) {
    z.begin();
  }
}

void LightController::update() {
  for (auto& z : zones_) {
    z.update();
  }
  if (impl_->alternate.isEnabled()) {
    impl_->routines.cancel();
  } else if (impl_->schedules.update(*this, impl_->alternate)) {
    impl_->routines.cancel();
  }
  impl_->routines.update(*this, impl_->settings.get());
  impl_->alternate.update(*this);
}

void LightController::startClock() { impl_->schedules.startNtp(); }

bool LightController::timeSynced() const { return impl_->schedules.timeSynced(); }

String LightController::currentTimeText() { return impl_->schedules.currentTimeText(); }

void LightController::onFirmwareUpdateStart() {
  impl_->routines.cancel();
  impl_->alternate.setEnabled(false, *this);
}

LightZone& LightController::zone(LightZoneId id) { return zones_[id]; }

const LightZone& LightController::zone(LightZoneId id) const { return zones_[id]; }

void LightController::setLightLevel(LightZoneId zoneId, uint8_t percent) {
  zones_[zoneId].setLightLevel(percent);
}

void LightController::setLightOn(LightZoneId zoneId) { zones_[zoneId].setLightOn(); }

void LightController::setLightOff(LightZoneId zoneId) { zones_[zoneId].setLightOff(); }

void LightController::fadeLight(LightZoneId zoneId, uint8_t targetPercent, uint32_t durationMs) {
  zones_[zoneId].fadeLight(targetPercent, durationMs);
}

void LightController::toggleLight(LightZoneId zoneId) { zones_[zoneId].toggleLight(); }

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

const LightSettings& LightController::settings() const { return impl_->settings.get(); }

bool LightController::applySettingsJson(JsonObject obj) {
  const uint8_t previousOnLevel = impl_->settings.get().defaultOnLevel;
  bool activeZones[ZONE_COUNT];
  for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
    activeZones[i] = zones_[i].isOn();
  }

  if (!impl_->settings.fromJson(obj)) {
    return false;
  }

  const LightSettings& cfg = impl_->settings.get();
  if (cfg.defaultOnLevel != previousOnLevel && !impl_->alternate.isEnabled() &&
      !impl_->routines.isActive()) {
    for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
      if (activeZones[i]) {
        fadeLight(static_cast<LightZoneId>(i), cfg.defaultOnLevel, cfg.fadeOnMs);
      }
    }
  }
  return true;
}

void LightController::settingsToJson(JsonObject obj) const { impl_->settings.toJson(obj); }

bool LightController::schedulesFromJson(JsonArray arr) { return impl_->schedules.fromJson(arr); }

void LightController::schedulesToJson(JsonArray arr) const { impl_->schedules.toJson(arr); }

RoutineId LightController::routineFromName(const char* name) const {
  return impl_->routines.idFromName(name);
}

bool LightController::routinesFromJson(JsonArray arr) {
  return impl_->routines.fromJson(arr);
}

void LightController::startRoutine(RoutineId id) { impl_->routines.start(id, *this); }

void LightController::stopRoutine() { impl_->routines.stop(*this); }

void LightController::toggleRoutine(RoutineId id) { impl_->routines.toggle(id, *this); }

void LightController::cancelRoutine() { impl_->routines.cancel(); }

bool LightController::routineActive() const { return impl_->routines.isActive(); }

RoutineId LightController::routineActiveId() const { return impl_->routines.active(); }

const char* LightController::routineStateName() const { return impl_->routines.stateName(); }

const char* LightController::routineStepName() const { return impl_->routines.stepName(); }

void LightController::routinesToJson(JsonDocument& doc, bool includeRoutineConfig) const {
  impl_->routines.legacyGroupJson(doc["group"].to<JsonObject>());
  impl_->routines.summaryJson(doc["routine"].to<JsonObject>());
  JsonArray routines = doc["routines"].to<JsonArray>();
  if (includeRoutineConfig) {
    impl_->routines.listJson(routines);
  } else {
    impl_->routines.listRuntimeJson(routines);
  }
}

void LightController::setAlternateEnabled(bool enabled) {
  if (enabled) {
    impl_->routines.cancel();
  }
  impl_->alternate.setEnabled(enabled, *this);
}

bool LightController::alternateEnabled() const { return impl_->alternate.isEnabled(); }

void LightController::setAlternatePeriodMs(uint32_t ms) { impl_->alternate.setPeriodMs(ms); }

void LightController::setAlternateLevel(uint8_t percent) { impl_->alternate.setLevel(percent); }

uint32_t LightController::alternatePeriodMs() const { return impl_->alternate.periodMs(); }

uint8_t LightController::alternateLevel() const { return impl_->alternate.level(); }

void LightController::alternateToJson(JsonObject obj) const {
  obj["enabled"] = impl_->alternate.isEnabled();
  obj["period"] = impl_->alternate.periodMs();
  obj["level"] = impl_->alternate.level();
}

void LightController::cancelAutomationForManualControl() {
  impl_->alternate.setEnabled(false, *this);
  impl_->routines.cancel();
}

void LightController::cancelAutomationIfFlashing() {
  impl_->routines.cancel();
  if (impl_->alternate.isEnabled()) {
    impl_->alternate.setEnabled(false, *this);
  }
}
