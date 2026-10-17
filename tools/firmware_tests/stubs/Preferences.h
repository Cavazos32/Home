#pragma once
// Stub de Preferences (NVS) en memoria; se comparte entre instancias para simular reinicios.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

class Preferences {
 public:
  static std::map<std::string, std::vector<uint8_t>>& store() {
    static std::map<std::string, std::vector<uint8_t>> s;
    return s;
  }
  static void clearAll() { store().clear(); }

  bool begin(const char*, bool) { return true; }
  uint32_t getUInt(const char* k, uint32_t def) { return get(k, def); }
  uint8_t getUChar(const char* k, uint8_t def) { return get(k, def); }
  float getFloat(const char* k, float def) { return get(k, def); }
  size_t putUInt(const char* k, uint32_t v) { return putBytes(k, &v, sizeof(v)); }
  size_t putUChar(const char* k, uint8_t v) { return putBytes(k, &v, sizeof(v)); }
  size_t putFloat(const char* k, float v) { return putBytes(k, &v, sizeof(v)); }

  size_t getBytesLength(const char* k) {
    auto it = store().find(k);
    return it == store().end() ? 0 : it->second.size();
  }
  size_t getBytes(const char* k, void* buf, size_t len) {
    auto it = store().find(k);
    if (it == store().end()) return 0;
    const size_t n = len < it->second.size() ? len : it->second.size();
    memcpy(buf, it->second.data(), n);
    return n;
  }
  size_t putBytes(const char* k, const void* buf, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(buf);
    store()[k] = std::vector<uint8_t>(p, p + len);
    return len;
  }
  bool remove(const char* k) { return store().erase(k) > 0; }

 private:
  template <typename T>
  T get(const char* k, T def) {
    auto it = store().find(k);
    if (it == store().end() || it->second.size() != sizeof(T)) return def;
    T v;
    memcpy(&v, it->second.data(), sizeof(T));
    return v;
  }
};
