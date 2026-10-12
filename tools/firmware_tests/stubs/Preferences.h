#pragma once
// Stub de Preferences (NVS): devuelve siempre los valores por defecto.

#include <cstddef>
#include <cstdint>

class Preferences {
 public:
  bool begin(const char*, bool) { return true; }
  uint32_t getUInt(const char*, uint32_t def) { return def; }
  uint8_t getUChar(const char*, uint8_t def) { return def; }
  float getFloat(const char*, float def) { return def; }
  size_t putUInt(const char*, uint32_t) { return 4; }
  size_t putUChar(const char*, uint8_t) { return 1; }
  size_t putFloat(const char*, float) { return 4; }
};
