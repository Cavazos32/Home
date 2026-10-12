#include "RoutineManager.h"
#include "LightSettings.h"

#ifndef BABY_SETUP_LEVEL
#define BABY_SETUP_LEVEL 40
#endif
#ifndef BABY_CUNA_LEVEL
#define BABY_CUNA_LEVEL 25
#endif
#ifndef ROUTINE_NEXT_STEP_AT
#define ROUTINE_NEXT_STEP_AT 0.70f
#endif

namespace {
struct RoutineStep {
  LightZoneId zone;
  uint8_t level;
};

struct RoutineDef {
  const char* name;
  uint8_t stepCount;
  RoutineStep steps[ZONE_COUNT];
};

const RoutineDef kRoutines[ROUTINE_COUNT] = {
    {"none", 0, {}},
    {"habitacion", 2, {{ZONE_CUNA, GROUP_CUNA_LEVEL}, {ZONE_SETUP, GROUP_SETUP_LEVEL}}},
    {"baby", 2, {{ZONE_SETUP, BABY_SETUP_LEVEL}, {ZONE_CUNA, BABY_CUNA_LEVEL}}},
};

// Margen en puntos de porcentaje para dar por cumplido un objetivo.
constexpr int kLevelTolerance = 1;

const char* zoneName(LightZoneId id) { return id == ZONE_CUNA ? "cuna" : "setup"; }

bool nearLevel(int a, int b) {
  const int d = a - b;
  return d <= kLevelTolerance && d >= -kLevelTolerance;
}

// La zona ya está en `level` o su fade en curso va hacia él.
bool headingTo(const LightZone& z, uint8_t level) {
  return nearLevel(z.isTransitioning() ? z.targetLevel() : z.currentLevel(), level);
}

bool headingOff(const LightZone& z) {
  return z.isTransitioning() ? z.targetLevel() == 0 : !z.isOn();
}
}  // namespace

RoutineId RoutineManager::idFromName(const char* name) {
  if (name == nullptr) return ROUTINE_NONE;
  for (uint8_t i = ROUTINE_NONE + 1; i < ROUTINE_COUNT; ++i) {
    if (strcmp(name, kRoutines[i].name) == 0) return static_cast<RoutineId>(i);
  }
  return ROUTINE_NONE;
}

const char* RoutineManager::name(RoutineId id) {
  return id < ROUTINE_COUNT ? kRoutines[id].name : "none";
}

void RoutineManager::start(RoutineId id, LightController& lights) {
  if (id == ROUTINE_NONE || id >= ROUTINE_COUNT) {
    cancel();
    return;
  }
  active_ = id;
  step_ = 0;
  completed_ = false;
  stepZone_ = kRoutines[id].steps[0].zone;
  update(lights);
}

void RoutineManager::stop(LightController& lights) {
  cancel();
  for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
    const LightZoneId id = static_cast<LightZoneId>(i);
    if (!headingOff(lights.zone(id))) lights.setLightOff(id);
  }
}

void RoutineManager::toggle(RoutineId id, LightController& lights) {
  if (id != ROUTINE_NONE && active_ == id) {
    stop(lights);
  } else {
    start(id, lights);
  }
}

void RoutineManager::cancel() {
  active_ = ROUTINE_NONE;
  step_ = 0;
  completed_ = false;
}

void RoutineManager::update(LightController& lights) {
  if (!running()) return;
  const RoutineDef& def = kRoutines[active_];

  while (step_ < def.stepCount) {
    const RoutineStep& s = def.steps[step_];
    const LightZone& z = lights.zone(s.zone);
    if (!headingTo(z, s.level)) {
      lights.fadeLight(s.zone, s.level, appSettings().get().fadeOnMs);
    }
    stepZone_ = s.zone;
    if (z.transitionProgress() < ROUTINE_NEXT_STEP_AT) return;
    ++step_;
  }

  for (int i = def.stepCount - 1; i >= 0; --i) {
    if (lights.zone(def.steps[i].zone).isTransitioning()) {
      stepZone_ = def.steps[i].zone;
      return;
    }
  }
  completed_ = true;
}

const char* RoutineManager::stateName() const {
  if (!isActive()) return "idle";
  return completed_ ? "completed" : "running";
}

const char* RoutineManager::stepName() const {
  return running() ? zoneName(stepZone_) : "idle";
}

void RoutineManager::summaryJson(JsonObject obj) const {
  obj["active"] = name(active_);
  obj["state"] = stateName();
  obj["step"] = stepName();
}

void RoutineManager::listJson(JsonArray arr) const {
  for (uint8_t i = ROUTINE_NONE + 1; i < ROUTINE_COUNT; ++i) {
    const RoutineDef& def = kRoutines[i];
    const bool mine = active_ == i;
    JsonObject o = arr.add<JsonObject>();
    o["id"] = def.name;
    o["active"] = mine;
    o["state"] = mine ? stateName() : "idle";
    o["step"] = mine ? stepName() : "idle";
    JsonArray steps = o["steps"].to<JsonArray>();
    for (uint8_t s = 0; s < def.stepCount; ++s) {
      JsonObject step = steps.add<JsonObject>();
      step["zone"] = zoneName(def.steps[s].zone);
      step["level"] = def.steps[s].level;
    }
  }
}

void RoutineManager::legacyGroupJson(JsonObject obj) const {
  const bool mine = active_ == ROUTINE_HABITACION;
  obj["id"] = "habitacion";
  obj["active"] = mine;
  obj["running"] = mine && running();
  obj["step"] = mine ? stepName() : "idle";
  obj["cunaLevel"] = GROUP_CUNA_LEVEL;
  obj["setupLevel"] = GROUP_SETUP_LEVEL;
}
