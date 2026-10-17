#pragma once

#define WIFI_SSID "Home_ZC"
#define WIFI_PASSWORD "R8mK42vP"

#define MDNS_HOSTNAME "habitacion"
#define OTA_PASSWORD "1234"

#define GPIO_LIGHT_CUNA 25
#define GPIO_LIGHT_SETUP 26

#define PWM_FREQUENCY 12000
#define PWM_RESOLUTION 12
#define PWM_MAX_DUTY ((1 << PWM_RESOLUTION) - 1)

#define FADE_GAMMA 2.2f
#define FADE_ON_MS 1100
#define FADE_OFF_MS 1400
#define FADE_TIME_MS FADE_ON_MS
#define DEFAULT_ON_LEVEL 50

#define GROUP_CUNA_LEVEL 35
#define GROUP_SETUP_LEVEL 100
#define BABY_SETUP_LEVEL 40
#define BABY_CUNA_LEVEL 25
#define ROUTINE_NEXT_STEP_AT 0.70f

#define GMT_OFFSET_SEC (-6 * 3600)
#define DAYLIGHT_OFFSET_SEC 0
#define MAX_SCHEDULES 10

#define WEATHER_LAT 26.09
#define WEATHER_LON -98.28

#define FW_VERSION "1.0.14"
#define OTA_MANIFEST_URL "https://github.com/Cavazos32/Home/releases/latest/download/version.json"
#define OTA_CHECK_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)
#define OTA_BOOT_DELAY_MS (30UL * 1000UL)
#define OTA_RETRY_MS (15UL * 60UL * 1000UL)
