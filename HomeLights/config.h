#pragma once

#define WIFI_SSID "IZZI-2416"
#define WIFI_PASSWORD "F0AF85382416"

#define MDNS_HOSTNAME "habitacion"

// Contraseña para subir firmware por WiFi (Arduino IDE → puerto de red)
#define OTA_PASSWORD "1234"

#define GPIO_LIGHT_CUNA 25
#define GPIO_LIGHT_SETUP 26

#define PWM_FREQUENCY 12000
#define PWM_RESOLUTION 12
#define PWM_MAX_DUTY ((1 << PWM_RESOLUTION) - 1)

// Valores por defecto (se pueden cambiar desde la web; se guardan en flash).
#define FADE_GAMMA 2.2f
#define FADE_ON_MS 1100
#define FADE_OFF_MS 1400
#define FADE_TIME_MS FADE_ON_MS
#define DEFAULT_ON_LEVEL 50

// Zona horaria: México centro (sin horario de verano automático en ESP32 clásico)
#define GMT_OFFSET_SEC (-6 * 3600)
#define DAYLIGHT_OFFSET_SEC 0

#define MAX_SCHEDULES 10

// ---------------------------------------------------------------------------
// OTA por descarga desde internet (el ESP32 busca y baja el firmware él solo)
// ---------------------------------------------------------------------------
// Versión de ESTE firmware. Súbela (1.0.0 -> 1.0.1) antes de compilar un release;
// el ESP32 solo se actualiza si version.json trae una versión MAYOR.
#define FW_VERSION "1.0.2"

#define WEATHER_LAT 26.09
#define WEATHER_LON -98.28

// Manifiesto con la última versión publicada (asset del último GitHub Release).
#define OTA_MANIFEST_URL "https://github.com/Cavazos32/Home/releases/latest/download/version.json"

// Cada cuánto revisar si hay versión nueva (6 h) y espera tras arrancar (30 s).
#define OTA_CHECK_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)
#define OTA_BOOT_DELAY_MS (30UL * 1000UL)
// Si una revisión falla (sin internet, 404...), reintentar en 15 min.
#define OTA_RETRY_MS (15UL * 60UL * 1000UL)
