#pragma once
// Stub mínimo de Arduino para compilar módulos del firmware en el PC.

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

uint32_t millis();
bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution);
bool ledcWrite(uint8_t pin, uint32_t duty);

class String : public std::string {
 public:
  String() = default;
  String(const char* s) : std::string(s ? s : "") {}
  String(const std::string& s) : std::string(s) {}
};

inline void configTime(long, int, const char*, const char*) {}
inline bool getLocalTime(struct tm*, uint32_t = 5000) { return false; }
