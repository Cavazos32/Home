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

#define LEDC_CHANNEL_CUNA 0
#define LEDC_CHANNEL_SETUP 1

// Zona horaria: México centro (sin horario de verano automático en ESP32 clásico)
#define GMT_OFFSET_SEC (-6 * 3600)
#define DAYLIGHT_OFFSET_SEC 0

#define MAX_SCHEDULES 10
