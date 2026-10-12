#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "LightController.h"

enum RoutineId : uint8_t {
  ROUTINE_NONE = 0,
  ROUTINE_HABITACION = 1,
  ROUTINE_BABY = 2,
  ROUTINE_COUNT = 3
};

/**
 * Rutinas de grupo (Habitación, Baby): encienden zonas en orden y empiezan la
 * siguiente cuando la anterior cubre ROUTINE_NEXT_STEP_AT de su recorrido.
 *
 * Una rutina activa sigue activa al terminar su secuencia ("completed") hasta
 * que se apaga, se activa otra o un comando manual/horario/alternado la cancela.
 * Solo puede haber una rutina activa a la vez.
 */
class RoutineManager {
 public:
  /** ROUTINE_NONE si el nombre no corresponde a ninguna rutina. */
  static RoutineId idFromName(const char* name);
  static const char* name(RoutineId id);

  /** Activa la rutina (sustituye a la que hubiera) y reevalúa los objetivos. */
  void start(RoutineId id, LightController& lights);
  /** Desactiva cualquier rutina y apaga ambas zonas con fade. */
  void stop(LightController& lights);
  /** Activa `id` si no es la rutina activa; si lo es, la desactiva y apaga. */
  void toggle(RoutineId id, LightController& lights);
  /** Olvida la rutina activa sin tocar las luces (control manual, horarios...). */
  void cancel();

  void update(LightController& lights);

  RoutineId active() const { return active_; }
  bool isActive() const { return active_ != ROUTINE_NONE; }
  bool running() const { return isActive() && !completed_; }
  /** "idle" | "running" | "completed" */
  const char* stateName() const;
  /** Zona que la secuencia está encendiendo ("cuna" | "setup") o "idle". */
  const char* stepName() const;

  /** {"active","state","step"} de la rutina activa. */
  void summaryJson(JsonObject obj) const;
  /** Lista con el estado y los pasos de cada rutina. */
  void listJson(JsonArray arr) const;
  /** Objeto "group" heredado (solo Habitación) para clientes anteriores. */
  void legacyGroupJson(JsonObject obj) const;

 private:
  RoutineId active_ = ROUTINE_NONE;
  uint8_t step_ = 0;
  bool completed_ = false;
  LightZoneId stepZone_ = ZONE_CUNA;
};
