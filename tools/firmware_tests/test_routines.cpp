// Pruebas de host para LightController (código real del firmware).
// Compilar y ejecutar con: python tools/firmware_tests/run.py

#include <cstdio>
#include <functional>

#include <Preferences.h>

#include "LightController.h"

static uint32_t g_now = 1000;
uint32_t millis() { return g_now; }
bool ledcAttach(uint8_t, uint32_t, uint8_t) { return true; }
bool ledcWrite(uint8_t, uint32_t) { return true; }

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    ++g_checks;                                                         \
    if (!(cond)) {                                                      \
      ++g_failures;                                                     \
      std::printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
    }                                                                   \
  } while (0)

static const uint32_t TICK_MS = 5;

struct Snap {
  bool tr = false;
  int target = 0;
  float prog = 1.0f;
};

struct Fixture {
  LightController lights;
  Snap last[ZONE_COUNT];
  int commands[ZONE_COUNT] = {0, 0};
  int minTarget[ZONE_COUNT] = {101, 101};
  int maxTarget[ZONE_COUNT] = {-1, -1};

  explicit Fixture(bool keepNvs = false) {
    if (!keepNvs) Preferences::clearAll();
    lights.begin();
    snapshot();
  }

  const LightZone& z(LightZoneId id) const { return lights.zone(id); }

  Snap snap(LightZoneId id) const {
    Snap s;
    s.tr = z(id).isTransitioning();
    s.target = z(id).targetLevel();
    s.prog = z(id).transitionProgress();
    return s;
  }

  void snapshot() {
    for (uint8_t i = 0; i < ZONE_COUNT; ++i) last[i] = snap(static_cast<LightZoneId>(i));
  }

  // Cuenta un "comando" cuando aparece un fade nuevo o se reinicia uno existente.
  void observe() {
    for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
      const Snap s = snap(static_cast<LightZoneId>(i));
      if (s.tr && (!last[i].tr || s.target != last[i].target || s.prog + 1e-4f < last[i].prog)) {
        ++commands[i];
        if (s.target < minTarget[i]) minTarget[i] = s.target;
        if (s.target > maxTarget[i]) maxTarget[i] = s.target;
      }
      last[i] = s;
    }
  }

  void resetCounters() {
    for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
      commands[i] = 0;
      minTarget[i] = 101;
      maxTarget[i] = -1;
    }
    snapshot();
  }

  // Igual que loop(): primero avanzan los fades, después la rutina.
  void tick() {
    g_now += TICK_MS;
    lights.update();
    observe();
  }

  void run(uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += TICK_MS) tick();
  }

  bool runUntil(const std::function<bool()>& pred, uint32_t maxMs = 10000) {
    for (uint32_t t = 0; t < maxMs; t += TICK_MS) {
      if (pred()) return true;
      tick();
    }
    return pred();
  }

  void start(RoutineId id) { lights.startRoutine(id); observe(); }
  void toggle(RoutineId id) { lights.toggleRoutine(id); observe(); }
  void stop() { lights.stopRoutine(); observe(); }

  void setStable(LightZoneId id, uint8_t level) {
    lights.setLightLevel(id, level);
    snapshot();
  }

  bool stableAt(LightZoneId id, int level) const {
    return !z(id).isTransitioning() && z(id).currentLevel() == level;
  }

  bool state(const char* name) const { return strcmp(lights.routineStateName(), name) == 0; }
  bool step(const char* name) const { return strcmp(lights.routineStepName(), name) == 0; }
};

// Avanza hasta que `next` empieza su fade y comprueba que ocurrió justo cuando
// `first` cruzó el 70 % de su recorrido, con `first` todavía en transición.
static void checkOverlap(Fixture& f, LightZoneId first, LightZoneId next) {
  float prevProg = f.z(first).transitionProgress();
  bool started = false;
  for (uint32_t t = 0; t < 5000; t += TICK_MS) {
    prevProg = f.z(first).transitionProgress();
    f.tick();
    if (f.z(next).isTransitioning()) {
      started = true;
      break;
    }
  }
  CHECK(started);
  CHECK(prevProg < ROUTINE_NEXT_STEP_AT);
  CHECK(f.z(first).transitionProgress() >= ROUTINE_NEXT_STEP_AT);
  CHECK(f.z(first).isTransitioning());
}

// ---------------------------------------------------------------- LightZone

static void test_progress_basics() {
  Fixture f;
  CHECK(f.z(ZONE_CUNA).transitionProgress() == 1.0f);

  f.setStable(ZONE_CUNA, 20);
  f.lights.fadeLight(ZONE_CUNA, 35, 1000);
  CHECK(f.z(ZONE_CUNA).transitionProgress() == 0.0f);
  f.run(500);
  const float expected = (f.z(ZONE_CUNA).brightnessPercent() - 20.0f) / 15.0f;
  const float diff = f.z(ZONE_CUNA).transitionProgress() - expected;
  CHECK(diff < 1e-3f && diff > -1e-3f);

  f.setStable(ZONE_SETUP, 80);
  f.lights.fadeLight(ZONE_SETUP, 40, 1000);
  f.run(400);
  CHECK(f.z(ZONE_SETUP).transitionProgress() > 0.0f);
  CHECK(f.z(ZONE_SETUP).transitionProgress() < 1.0f);

  f.setStable(ZONE_SETUP, 35);
  f.lights.fadeLight(ZONE_SETUP, 35, 1000);
  CHECK(f.z(ZONE_SETUP).transitionProgress() == 1.0f);
}

// --------------------------------------------------------------- Habitación

static void test_hab_both_off() {
  Fixture f;
  f.start(ROUTINE_HABITACION);
  CHECK(f.state("running"));
  CHECK(f.step("cuna"));
  CHECK(f.z(ZONE_CUNA).isTransitioning() && f.z(ZONE_CUNA).targetLevel() == GROUP_CUNA_LEVEL);
  CHECK(!f.z(ZONE_SETUP).isTransitioning() && !f.z(ZONE_SETUP).isOn());
  checkOverlap(f, ZONE_CUNA, ZONE_SETUP);
  CHECK(f.z(ZONE_SETUP).targetLevel() == GROUP_SETUP_LEVEL);
  CHECK(f.step("setup"));
  f.run(3000);
  CHECK(f.state("completed"));
  CHECK(f.stableAt(ZONE_CUNA, GROUP_CUNA_LEVEL));
  CHECK(f.stableAt(ZONE_SETUP, GROUP_SETUP_LEVEL));
  CHECK(f.commands[ZONE_CUNA] == 1);
  CHECK(f.commands[ZONE_SETUP] == 1);
}

static void test_hab_cuna_at_target() {
  Fixture f;
  f.setStable(ZONE_CUNA, GROUP_CUNA_LEVEL);
  f.start(ROUTINE_HABITACION);
  CHECK(f.z(ZONE_SETUP).isTransitioning());
  CHECK(f.step("setup"));
  f.run(3000);
  CHECK(f.state("completed"));
  CHECK(f.commands[ZONE_CUNA] == 0);
  CHECK(f.commands[ZONE_SETUP] == 1);
  CHECK(f.stableAt(ZONE_CUNA, GROUP_CUNA_LEVEL));
  CHECK(f.stableAt(ZONE_SETUP, GROUP_SETUP_LEVEL));
}

static void test_hab_cuna_partial() {
  Fixture f;
  f.setStable(ZONE_CUNA, 20);
  f.start(ROUTINE_HABITACION);
  CHECK(f.z(ZONE_CUNA).targetLevel() == GROUP_CUNA_LEVEL);
  checkOverlap(f, ZONE_CUNA, ZONE_SETUP);
  const float threshold = 20.0f + ROUTINE_NEXT_STEP_AT * (GROUP_CUNA_LEVEL - 20);
  CHECK(f.z(ZONE_CUNA).brightnessPercent() >= threshold - 0.01f);
  f.run(3000);
  CHECK(f.stableAt(ZONE_CUNA, GROUP_CUNA_LEVEL));
  CHECK(f.stableAt(ZONE_SETUP, GROUP_SETUP_LEVEL));
  CHECK(f.commands[ZONE_CUNA] == 1);
}

static void test_hab_cuna_transitioning_elsewhere() {
  Fixture f;
  f.lights.fadeLight(ZONE_CUNA, 80, 1100);
  f.run(300);
  f.resetCounters();
  f.start(ROUTINE_HABITACION);
  CHECK(f.commands[ZONE_CUNA] == 1);
  CHECK(f.z(ZONE_CUNA).targetLevel() == GROUP_CUNA_LEVEL);
  checkOverlap(f, ZONE_CUNA, ZONE_SETUP);
  f.run(3000);
  CHECK(f.stableAt(ZONE_CUNA, GROUP_CUNA_LEVEL));
  CHECK(f.stableAt(ZONE_SETUP, GROUP_SETUP_LEVEL));
}

static void test_hab_cuna_transitioning_to_target() {
  Fixture f;
  f.lights.fadeLight(ZONE_CUNA, GROUP_CUNA_LEVEL, 1100);
  f.run(300);
  const float before = f.z(ZONE_CUNA).transitionProgress();
  f.resetCounters();
  f.start(ROUTINE_HABITACION);
  CHECK(f.commands[ZONE_CUNA] == 0);
  CHECK(f.z(ZONE_CUNA).transitionProgress() >= before);
  f.run(3000);
  CHECK(f.commands[ZONE_CUNA] == 0);
  CHECK(f.state("completed"));
  CHECK(f.stableAt(ZONE_SETUP, GROUP_SETUP_LEVEL));
}

static void test_hab_setup_on_cuna_off() {
  Fixture f;
  f.setStable(ZONE_SETUP, GROUP_SETUP_LEVEL);
  f.start(ROUTINE_HABITACION);
  f.run(3000);
  CHECK(f.state("completed"));
  CHECK(f.commands[ZONE_SETUP] == 0);
  CHECK(f.stableAt(ZONE_CUNA, GROUP_CUNA_LEVEL));
  CHECK(f.stableAt(ZONE_SETUP, GROUP_SETUP_LEVEL));
}

static void test_hab_setup_partial_waits_its_turn() {
  Fixture f;
  f.setStable(ZONE_SETUP, 60);
  f.start(ROUTINE_HABITACION);
  CHECK(!f.z(ZONE_SETUP).isTransitioning());
  checkOverlap(f, ZONE_CUNA, ZONE_SETUP);
  f.run(3000);
  CHECK(f.stableAt(ZONE_SETUP, GROUP_SETUP_LEVEL));
  CHECK(f.commands[ZONE_SETUP] == 1);
}

static void test_hab_both_at_targets() {
  Fixture f;
  f.setStable(ZONE_CUNA, GROUP_CUNA_LEVEL);
  f.setStable(ZONE_SETUP, GROUP_SETUP_LEVEL);
  f.start(ROUTINE_HABITACION);
  CHECK(f.state("completed"));
  f.run(2000);
  CHECK(f.commands[ZONE_CUNA] == 0);
  CHECK(f.commands[ZONE_SETUP] == 0);
}

static void test_hab_toggle_first_fade() {
  Fixture f;
  f.start(ROUTINE_HABITACION);
  f.run(200);
  f.toggle(ROUTINE_HABITACION);
  CHECK(f.state("idle"));
  CHECK(f.z(ZONE_CUNA).isTransitioning() && f.z(ZONE_CUNA).targetLevel() == 0);
  CHECK(!f.z(ZONE_SETUP).isTransitioning());
  f.run(3000);
  CHECK(f.stableAt(ZONE_CUNA, 0));
  CHECK(f.stableAt(ZONE_SETUP, 0));
  CHECK(f.commands[ZONE_SETUP] == 0);
  CHECK(f.state("idle"));
}

static void test_hab_toggle_second_fade() {
  Fixture f;
  f.start(ROUTINE_HABITACION);
  CHECK(f.runUntil([&] { return f.z(ZONE_SETUP).isTransitioning(); }));
  f.run(100);
  f.toggle(ROUTINE_HABITACION);
  CHECK(f.state("idle"));
  CHECK(f.z(ZONE_CUNA).targetLevel() == 0);
  CHECK(f.z(ZONE_SETUP).targetLevel() == 0);
  f.run(3000);
  CHECK(f.stableAt(ZONE_CUNA, 0));
  CHECK(f.stableAt(ZONE_SETUP, 0));
  CHECK(f.state("idle"));
}

static void test_hab_toggle_after_complete() {
  Fixture f;
  f.start(ROUTINE_HABITACION);
  f.run(3000);
  CHECK(f.state("completed"));
  CHECK(f.lights.routineActiveId() == ROUTINE_HABITACION);
  f.toggle(ROUTINE_HABITACION);
  CHECK(f.state("idle"));
  f.run(3000);
  CHECK(f.stableAt(ZONE_CUNA, 0));
  CHECK(f.stableAt(ZONE_SETUP, 0));
}

static void test_hab_restart_while_running_keeps_fades() {
  Fixture f;
  f.start(ROUTINE_HABITACION);
  f.run(300);
  f.start(ROUTINE_HABITACION);
  f.run(3000);
  CHECK(f.commands[ZONE_CUNA] == 1);
  CHECK(f.commands[ZONE_SETUP] == 1);
  CHECK(f.state("completed"));
}

// --------------------------------------------------------------------- Baby

static void test_baby_both_off() {
  Fixture f;
  f.start(ROUTINE_BABY);
  CHECK(f.step("setup"));
  CHECK(f.z(ZONE_SETUP).targetLevel() == BABY_SETUP_LEVEL);
  CHECK(!f.z(ZONE_CUNA).isTransitioning());
  checkOverlap(f, ZONE_SETUP, ZONE_CUNA);
  CHECK(f.z(ZONE_CUNA).targetLevel() == BABY_CUNA_LEVEL);
  CHECK(f.step("cuna"));
  f.run(3000);
  CHECK(f.state("completed"));
  CHECK(f.stableAt(ZONE_SETUP, BABY_SETUP_LEVEL));
  CHECK(f.stableAt(ZONE_CUNA, BABY_CUNA_LEVEL));
  CHECK(f.commands[ZONE_SETUP] == 1 && f.commands[ZONE_CUNA] == 1);
}

static void test_baby_setup_at_target() {
  Fixture f;
  f.setStable(ZONE_SETUP, BABY_SETUP_LEVEL);
  f.start(ROUTINE_BABY);
  CHECK(f.z(ZONE_CUNA).isTransitioning());
  f.run(3000);
  CHECK(f.commands[ZONE_SETUP] == 0);
  CHECK(f.stableAt(ZONE_SETUP, BABY_SETUP_LEVEL));
  CHECK(f.stableAt(ZONE_CUNA, BABY_CUNA_LEVEL));
}

static void test_baby_setup_other_level() {
  Fixture f;
  f.setStable(ZONE_SETUP, 80);
  f.start(ROUTINE_BABY);
  CHECK(f.z(ZONE_SETUP).targetLevel() == BABY_SETUP_LEVEL);
  checkOverlap(f, ZONE_SETUP, ZONE_CUNA);
  const float threshold = 80.0f - ROUTINE_NEXT_STEP_AT * (80 - BABY_SETUP_LEVEL);
  CHECK(f.z(ZONE_SETUP).brightnessPercent() <= threshold + 0.01f);
  f.run(3000);
  CHECK(f.stableAt(ZONE_SETUP, BABY_SETUP_LEVEL));
  CHECK(f.stableAt(ZONE_CUNA, BABY_CUNA_LEVEL));
  CHECK(f.commands[ZONE_SETUP] == 1);
}

static void test_baby_setup_transitioning() {
  Fixture f;
  f.lights.fadeLight(ZONE_SETUP, BABY_SETUP_LEVEL, 1100);
  f.run(300);
  f.resetCounters();
  f.start(ROUTINE_BABY);
  CHECK(f.commands[ZONE_SETUP] == 0);
  checkOverlap(f, ZONE_SETUP, ZONE_CUNA);
  f.run(3000);
  CHECK(f.commands[ZONE_SETUP] == 0);
  CHECK(f.state("completed"));
}

static void test_baby_cuna_at_target() {
  Fixture f;
  f.setStable(ZONE_CUNA, BABY_CUNA_LEVEL);
  f.start(ROUTINE_BABY);
  f.run(3000);
  CHECK(f.commands[ZONE_CUNA] == 0);
  CHECK(f.stableAt(ZONE_CUNA, BABY_CUNA_LEVEL));
  CHECK(f.stableAt(ZONE_SETUP, BABY_SETUP_LEVEL));
}

static void test_baby_cuna_other_level_waits() {
  Fixture f;
  f.setStable(ZONE_CUNA, 60);
  f.start(ROUTINE_BABY);
  CHECK(!f.z(ZONE_CUNA).isTransitioning());
  checkOverlap(f, ZONE_SETUP, ZONE_CUNA);
  f.run(3000);
  CHECK(f.stableAt(ZONE_CUNA, BABY_CUNA_LEVEL));
  CHECK(f.minTarget[ZONE_CUNA] == BABY_CUNA_LEVEL);
}

static void test_baby_toggle_during() {
  Fixture f;
  f.start(ROUTINE_BABY);
  f.run(300);
  f.toggle(ROUTINE_BABY);
  CHECK(f.state("idle"));
  f.run(3000);
  CHECK(f.stableAt(ZONE_SETUP, 0));
  CHECK(f.stableAt(ZONE_CUNA, 0));
  CHECK(f.commands[ZONE_CUNA] == 0);
}

static void test_baby_toggle_after_complete() {
  Fixture f;
  f.start(ROUTINE_BABY);
  f.run(3000);
  CHECK(f.state("completed"));
  f.toggle(ROUTINE_BABY);
  CHECK(f.state("idle"));
  f.run(3000);
  CHECK(f.stableAt(ZONE_SETUP, 0));
  CHECK(f.stableAt(ZONE_CUNA, 0));
}

// ------------------------------------------------------- Interacción modos

static void test_baby_while_hab_running() {
  Fixture f;
  f.start(ROUTINE_HABITACION);
  f.run(300);
  f.resetCounters();
  f.toggle(ROUTINE_BABY);
  CHECK(f.lights.routineActiveId() == ROUTINE_BABY);
  CHECK(f.z(ZONE_SETUP).targetLevel() == BABY_SETUP_LEVEL);
  f.run(4000);
  CHECK(f.state("completed"));
  CHECK(f.maxTarget[ZONE_SETUP] == BABY_SETUP_LEVEL);
  CHECK(f.minTarget[ZONE_CUNA] >= BABY_CUNA_LEVEL);
  CHECK(f.stableAt(ZONE_SETUP, BABY_SETUP_LEVEL));
  CHECK(f.stableAt(ZONE_CUNA, BABY_CUNA_LEVEL));
}

static void test_hab_while_baby_running() {
  Fixture f;
  f.start(ROUTINE_BABY);
  CHECK(f.runUntil([&] { return f.z(ZONE_CUNA).isTransitioning(); }));
  f.resetCounters();
  f.toggle(ROUTINE_HABITACION);
  CHECK(f.lights.routineActiveId() == ROUTINE_HABITACION);
  CHECK(f.z(ZONE_CUNA).targetLevel() == GROUP_CUNA_LEVEL);
  f.run(4000);
  CHECK(f.state("completed"));
  CHECK(f.minTarget[ZONE_CUNA] == GROUP_CUNA_LEVEL);
  CHECK(f.minTarget[ZONE_SETUP] >= BABY_SETUP_LEVEL);
  CHECK(f.stableAt(ZONE_CUNA, GROUP_CUNA_LEVEL));
  CHECK(f.stableAt(ZONE_SETUP, GROUP_SETUP_LEVEL));
}

static void test_switch_from_completed_no_blackout() {
  Fixture f;
  f.start(ROUTINE_HABITACION);
  f.run(3000);
  f.resetCounters();
  f.toggle(ROUTINE_BABY);
  CHECK(f.lights.routineActiveId() == ROUTINE_BABY);
  CHECK(f.state("running"));
  f.run(4000);
  CHECK(f.minTarget[ZONE_CUNA] > 0);
  CHECK(f.minTarget[ZONE_SETUP] > 0);
  CHECK(f.stableAt(ZONE_SETUP, BABY_SETUP_LEVEL));
  CHECK(f.stableAt(ZONE_CUNA, BABY_CUNA_LEVEL));

  f.resetCounters();
  f.toggle(ROUTINE_HABITACION);
  f.run(4000);
  CHECK(f.minTarget[ZONE_CUNA] > 0);
  CHECK(f.minTarget[ZONE_SETUP] > 0);
  CHECK(f.stableAt(ZONE_CUNA, GROUP_CUNA_LEVEL));
  CHECK(f.stableAt(ZONE_SETUP, GROUP_SETUP_LEVEL));
}

static void test_manual_command_cancels() {
  Fixture f;
  f.start(ROUTINE_HABITACION);
  f.run(300);
  // Equivale a handleCommand: cancela la rutina y aplica el comando manual.
  f.lights.cancelRoutine();
  f.lights.fadeLight(ZONE_SETUP, 10, 500);
  f.observe();
  f.resetCounters();
  f.run(4000);
  CHECK(f.state("idle"));
  CHECK(f.stableAt(ZONE_SETUP, 10));
  CHECK(f.commands[ZONE_SETUP] == 0);
  CHECK(f.commands[ZONE_CUNA] == 0);
}

static void test_cancel_after_complete_keeps_lights() {
  Fixture f;
  f.start(ROUTINE_BABY);
  f.run(3000);
  f.lights.cancelRoutine();
  f.resetCounters();
  f.run(1000);
  CHECK(f.state("idle"));
  CHECK(f.stableAt(ZONE_SETUP, BABY_SETUP_LEVEL));
  CHECK(f.stableAt(ZONE_CUNA, BABY_CUNA_LEVEL));
  CHECK(f.commands[ZONE_SETUP] == 0 && f.commands[ZONE_CUNA] == 0);
}

static void test_stop_when_idle_turns_off() {
  Fixture f;
  f.setStable(ZONE_CUNA, 50);
  f.stop();
  CHECK(f.state("idle"));
  CHECK(f.z(ZONE_CUNA).targetLevel() == 0);
  CHECK(!f.z(ZONE_SETUP).isTransitioning());
  f.run(2000);
  CHECK(f.stableAt(ZONE_CUNA, 0));
}

static int countActive(const LightController& lights) {
  JsonDocument doc;
  lights.routinesToJson(doc);
  JsonArray arr = doc["routines"].as<JsonArray>();
  int n = 0;
  for (JsonObject o : arr) {
    if (o["active"].as<bool>()) ++n;
    if (!o["active"].as<bool>()) {
      if (strcmp(o["state"] | "", "idle") != 0) ++n;
    }
  }
  return n;
}

static void test_never_two_active() {
  Fixture f;
  uint32_t seed = 12345;
  auto rnd = [&]() { seed = seed * 1103515245u + 12345u; return (seed >> 16) & 0x7fff; };
  for (int i = 0; i < 2000; ++i) {
    switch (rnd() % 7) {
      case 0: f.toggle(ROUTINE_HABITACION); break;
      case 1: f.toggle(ROUTINE_BABY); break;
      case 2: f.start(ROUTINE_HABITACION); break;
      case 3: f.start(ROUTINE_BABY); break;
      case 4: f.stop(); break;
      case 5: f.lights.cancelRoutine(); break;
      default: break;
    }
    f.run((rnd() % 40) * TICK_MS);
    CHECK(countActive(f.lights) <= 1);
    CHECK(f.lights.routineActive() || f.state("idle"));
  }
}

static void test_json_shapes() {
  Fixture f;
  f.start(ROUTINE_BABY);
  JsonDocument doc;
  f.lights.routinesToJson(doc);
  CHECK(strcmp(doc["routine"]["active"] | "", "baby") == 0);
  CHECK(strcmp(doc["routine"]["state"] | "", "running") == 0);
  CHECK(strcmp(doc["routine"]["step"] | "", "setup") == 0);
  CHECK(doc["routines"].size() == 2);
  CHECK(strcmp(doc["routines"][0]["id"] | "", "habitacion") == 0);
  CHECK(strcmp(doc["routines"][0]["state"] | "", "idle") == 0);
  CHECK(strcmp(doc["routines"][1]["id"] | "", "baby") == 0);
  CHECK(doc["routines"][1]["active"].as<bool>());
  CHECK(strcmp(doc["routines"][1]["steps"][0]["zone"] | "", "setup") == 0);
  CHECK(doc["routines"][1]["steps"][0]["level"].as<int>() == BABY_SETUP_LEVEL);
  CHECK(doc["routines"][1]["steps"][1]["level"].as<int>() == BABY_CUNA_LEVEL);
  CHECK(strcmp(doc["group"]["id"] | "", "habitacion") == 0);
  CHECK(!doc["group"]["running"].as<bool>());
  CHECK(strcmp(doc["group"]["step"] | "", "idle") == 0);
  CHECK(doc["group"]["cunaLevel"].as<int>() == GROUP_CUNA_LEVEL);

  f.start(ROUTINE_HABITACION);
  JsonDocument doc2;
  f.lights.routinesToJson(doc2);
  CHECK(doc2["group"]["running"].as<bool>());
  CHECK(doc2["group"]["active"].as<bool>());
  CHECK(strcmp(doc2["group"]["step"] | "", "cuna") == 0);

  CHECK(f.lights.routineFromName("habitacion") == ROUTINE_HABITACION);
  CHECK(f.lights.routineFromName("baby") == ROUTINE_BABY);
  CHECK(f.lights.routineFromName("none") == ROUTINE_NONE);
  CHECK(f.lights.routineFromName("Baby") == ROUTINE_NONE);
  CHECK(f.lights.routineFromName(nullptr) == ROUTINE_NONE);
}

static void addGroup(JsonArray arr, const char* id, const char* label, int cunaLevel) {
  JsonObject o = arr.add<JsonObject>();
  o["id"] = id;
  o["label"] = label;
  o["nextStepAt"] = 0.7;
  JsonObject s = o["steps"].to<JsonArray>().add<JsonObject>();
  s["zone"] = "cuna";
  s["level"] = cunaLevel;
}

static int routineCount(LightController& lights) {
  JsonDocument doc;
  lights.routinesToJson(doc);
  return static_cast<int>(doc["routines"].size());
}

static void test_save_new_group_keeps_all() {
  Fixture f;
  JsonDocument in;
  JsonArray arr = in["routines"].to<JsonArray>();
  addGroup(arr, "habitacion", "Habitación", 35);
  addGroup(arr, "baby", "Baby", 25);
  addGroup(arr, "lectura", "Lectura", 30);
  CHECK(f.lights.routinesFromJson(arr));
  CHECK(routineCount(f.lights) == 3);
  CHECK(f.lights.routineFromName("baby") == ROUTINE_BABY);
  const RoutineId lectura = f.lights.routineFromName("lectura");
  CHECK(lectura != ROUTINE_NONE);

  f.start(lectura);
  CHECK(f.lights.routineActiveId() == lectura);
  f.run(5000);
  CHECK(f.z(ZONE_CUNA).targetLevel() == 30);

  Fixture rebooted(true);
  CHECK(routineCount(rebooted.lights) == 3);
  CHECK(rebooted.lights.routineFromName("lectura") != ROUTINE_NONE);
  CHECK(rebooted.lights.routineFromName("baby") == ROUTINE_BABY);
}

static void test_builtin_groups_restored() {
  Fixture f;
  JsonDocument in;
  JsonArray arr = in["routines"].to<JsonArray>();
  addGroup(arr, "lectura", "Lectura", 30);
  CHECK(f.lights.routinesFromJson(arr));
  CHECK(f.lights.routineFromName("habitacion") == ROUTINE_HABITACION);
  CHECK(f.lights.routineFromName("baby") == ROUTINE_BABY);
  CHECK(f.lights.routineFromName("lectura") != ROUTINE_NONE);
  CHECK(routineCount(f.lights) == 3);
}

static void test_rejects_too_many_groups() {
  Fixture f;
  JsonDocument in;
  JsonArray arr = in["routines"].to<JsonArray>();
  const char* ids[] = {"habitacion", "baby", "g3", "g4", "g5", "g6"};
  for (const char* id : ids) addGroup(arr, id, id, 10);
  CHECK(!f.lights.routinesFromJson(arr));
  CHECK(routineCount(f.lights) == 2);
}

int main() {
  struct Case { const char* name; void (*fn)(); };
  const Case cases[] = {
      {"progreso de LightZone", test_progress_basics},
      {"01 Habitación: ambas apagadas", test_hab_both_off},
      {"02 Habitación: cuna en objetivo, setup apagado", test_hab_cuna_at_target},
      {"03 Habitación: cuna parcial (20%), setup apagado", test_hab_cuna_partial},
      {"04a Habitación: cuna en transición hacia otro nivel", test_hab_cuna_transitioning_elsewhere},
      {"04b Habitación: cuna en transición hacia su objetivo", test_hab_cuna_transitioning_to_target},
      {"05a Habitación: setup encendido (objetivo), cuna apagada", test_hab_setup_on_cuna_off},
      {"05b Habitación: setup a otro nivel espera su turno", test_hab_setup_partial_waits_its_turn},
      {"06 Habitación: ambas en objetivo", test_hab_both_at_targets},
      {"07 Habitación: toggle en el primer fade", test_hab_toggle_first_fade},
      {"08 Habitación: toggle en el segundo fade", test_hab_toggle_second_fade},
      {"09 Habitación: toggle tras completar", test_hab_toggle_after_complete},
      {"   Habitación: enabled:true repetido no reinicia fades", test_hab_restart_while_running_keeps_fades},
      {"10 Baby: ambas apagadas", test_baby_both_off},
      {"11 Baby: setup estable al 40%", test_baby_setup_at_target},
      {"12 Baby: setup a otro nivel (80%)", test_baby_setup_other_level},
      {"13 Baby: setup en transición", test_baby_setup_transitioning},
      {"14a Baby: cuna estable al 25%", test_baby_cuna_at_target},
      {"14b Baby: cuna a otro nivel espera su turno", test_baby_cuna_other_level_waits},
      {"15 Baby: toggle durante la secuencia", test_baby_toggle_during},
      {"16 Baby: toggle tras completar", test_baby_toggle_after_complete},
      {"17 Baby mientras Habitación en curso", test_baby_while_hab_running},
      {"18 Habitación mientras Baby en curso", test_hab_while_baby_running},
      {"19 Cambiar de modo completado sin apagón", test_switch_from_completed_no_blackout},
      {"20 Comando manual cancela la rutina", test_manual_command_cancels},
      {"   cancel() tras completar conserva luces", test_cancel_after_complete_keeps_lights},
      {"   enabled:false sin rutina apaga", test_stop_when_idle_turns_off},
      {"22 Nunca dos rutinas activas (aleatorio)", test_never_two_active},
      {"   Formato JSON de estado", test_json_shapes},
      {"   Guardar grupo nuevo conserva todos y se activa", test_save_new_group_keeps_all},
      {"   Habitación y Baby se restauran si faltan", test_builtin_groups_restored},
      {"   Rechaza más grupos de los que caben", test_rejects_too_many_groups},
  };
  for (const Case& c : cases) {
    const int before = g_failures;
    c.fn();
    std::printf("%s %s\n", g_failures == before ? "OK  " : "FAIL", c.name);
  }
  std::printf("\n%d comprobaciones, %d fallos\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
