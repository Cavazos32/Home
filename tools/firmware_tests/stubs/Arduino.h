#pragma once
// Stub mínimo de Arduino para compilar módulos del firmware en el PC.

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

uint32_t millis();
bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution);
bool ledcWrite(uint8_t pin, uint32_t duty);
