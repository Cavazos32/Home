#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#include "config.h"
#include "LightController.h"
#include "RemoteUpdate.h"
#include "index.h"

LightController lights;
WebServer server(80);

String deviceIp;

static void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

static void handleApiOptions() {
  sendCorsHeaders();
  server.send(204);
}

static void sendJson(int code, const String& body) {
  sendCorsHeaders();
  server.send(code, "application/json", body);
}

static LightZoneId zoneFromName(const char* name) {
  if (strcmp(name, "cuna") == 0) return ZONE_CUNA;
  if (strcmp(name, "setup") == 0) return ZONE_SETUP;
  return ZONE_COUNT;
}

static void fillZoneJson(JsonObject obj, const LightZone& z) {
  obj["level"] = z.currentLevel();
  obj["levelFine"] = z.brightnessPercent();
  obj["targetLevel"] = z.targetLevel();
  obj["on"] = z.isOn();
  obj["lit"] = z.isLit();
  obj["transitioning"] = z.isTransitioning();
  obj["phase"] = z.phaseName();
}

static bool serializeDocToString(JsonDocument& doc, String& out) {
  out.clear();
  if (doc.overflowed()) return false;
  serializeJson(doc, out);
  return !doc.overflowed() && !out.isEmpty() && out.charAt(0) == '{';
}

void handleIndexPage() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleState() {
  JsonDocument doc;
  doc["ip"] = deviceIp;
  doc["hostname"] = String(MDNS_HOSTNAME) + ".local";
  doc["clock"] = lights.currentTimeText();
  doc["timeSynced"] = lights.timeSynced();
  doc["weatherLat"] = WEATHER_LAT;
  doc["weatherLon"] = WEATHER_LON;
  fillZoneJson(doc["cuna"].to<JsonObject>(), lights.zone(ZONE_CUNA));
  fillZoneJson(doc["setup"].to<JsonObject>(), lights.zone(ZONE_SETUP));

  lights.routinesToJson(doc);

  lights.alternateToJson(doc["alternate"].to<JsonObject>());

  lights.schedulesToJson(doc["schedules"].to<JsonArray>());
  lights.settingsToJson(doc["settings"].to<JsonObject>());

  String out;
  if (!serializeDocToString(doc, out)) {
    sendJson(500, "{\"error\":\"state json overflow\"}");
    return;
  }
  sendJson(200, out);
}

void handleSettings() {
  if (!server.hasArg("plain")) {
    sendJson(400, "{\"error\":\"missing body\"}");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendJson(400, "{\"error\":\"invalid json\"}");
    return;
  }

  JsonObject obj = doc["settings"].as<JsonObject>();
  if (obj.isNull()) {
    sendJson(400, "{\"error\":\"invalid settings\"}");
    return;
  }

  if (!lights.applySettingsJson(obj)) {
    sendJson(400, "{\"error\":\"invalid settings\"}");
    return;
  }

  JsonDocument outDoc;
  outDoc["ok"] = true;
  lights.settingsToJson(outDoc["settings"].to<JsonObject>());
  String out;
  serializeJson(outDoc, out);
  sendJson(200, out);
}

void handleSchedule() {
  if (!server.hasArg("plain")) {
    sendJson(400, "{\"error\":\"missing body\"}");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendJson(400, "{\"error\":\"invalid json\"}");
    return;
  }

  JsonArray arr = doc["schedules"].as<JsonArray>();
  if (arr.isNull() || !lights.schedulesFromJson(arr)) {
    sendJson(400, "{\"error\":\"invalid schedules\"}");
    return;
  }

  JsonDocument outDoc;
  outDoc["ok"] = true;
  lights.schedulesToJson(outDoc["schedules"].to<JsonArray>());
  String out;
  serializeJson(outDoc, out);
  sendJson(200, out);
}

void handleRoutines() {
  if (!server.hasArg("plain")) {
    sendJson(400, "{\"error\":\"missing body\"}");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendJson(400, "{\"error\":\"invalid json\"}");
    return;
  }

  JsonArray arr = doc["routines"].as<JsonArray>();
  if (arr.isNull() || !lights.routinesFromJson(arr)) {
    sendJson(400, "{\"error\":\"invalid routines\"}");
    return;
  }

  JsonDocument outDoc;
  outDoc["ok"] = true;
  lights.routinesToJson(outDoc);
  String out;
  serializeJson(outDoc, out);
  sendJson(200, out);
}

void handleAlternate() {
  if (!server.hasArg("plain")) {
    sendJson(400, "{\"error\":\"missing body\"}");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendJson(400, "{\"error\":\"invalid json\"}");
    return;
  }

  uint32_t period = lights.alternatePeriodMs();
  uint8_t level = lights.alternateLevel();
  if (!doc["period"].isNull()) {
    const int value = doc["period"].as<int>();
    if (value < 0 || value > 5000) {
      sendJson(400, "{\"error\":\"invalid alternate period\"}");
      return;
    }
    period = static_cast<uint32_t>(value);
  }
  if (!doc["level"].isNull()) {
    const int value = doc["level"].as<int>();
    if (value < 0 || value > 100) {
      sendJson(400, "{\"error\":\"invalid alternate level\"}");
      return;
    }
    level = static_cast<uint8_t>(value);
  }

  lights.setAlternatePeriodMs(period);
  lights.setAlternateLevel(level);
  if (!doc["enabled"].isNull()) {
    lights.setAlternateEnabled(doc["enabled"].as<bool>());
  }

  JsonDocument outDoc;
  outDoc["ok"] = true;
  lights.alternateToJson(outDoc["alternate"].to<JsonObject>());
  String out;
  serializeJson(outDoc, out);
  sendJson(200, out);
}

void handleCommand() {
  if (!server.hasArg("plain")) {
    sendJson(400, "{\"error\":\"missing body\"}");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendJson(400, "{\"error\":\"invalid json\"}");
    return;
  }

  const char* action = doc["action"] | "level";

  if (strcmp(action, "group") == 0) {
    JsonVariant groupVar = doc["group"];
    if (!groupVar.isNull() && !groupVar.is<const char*>()) {
      sendJson(400, "{\"error\":\"invalid group\"}");
      return;
    }
    const RoutineId id = lights.routineFromName(groupVar | "habitacion");
    if (id == ROUTINE_NONE) {
      sendJson(400, "{\"error\":\"unknown group\"}");
      return;
    }

    enum { GROUP_OFF, GROUP_ON, GROUP_TOGGLE } op;
    JsonVariant enabled = doc["enabled"];
    if (enabled.is<bool>()) {
      op = enabled.as<bool>() ? GROUP_ON : GROUP_OFF;
    } else if (enabled.is<const char*>() &&
               strcmp(enabled.as<const char*>(), "toggle") == 0) {
      op = GROUP_TOGGLE;
    } else {
      sendJson(400, "{\"error\":\"invalid enabled (use true, false or \\\"toggle\\\")\"}");
      return;
    }

    lights.setAlternateEnabled(false);
    if (op == GROUP_TOGGLE) {
      lights.toggleRoutine(id);
    } else if (op == GROUP_ON) {
      lights.startRoutine(id);
    } else {
      lights.stopRoutine();
    }

    JsonDocument outDoc;
    outDoc["ok"] = true;
    lights.routinesToJson(outDoc);
    String out;
    serializeJson(outDoc, out);
    sendJson(200, out);
    return;
  }

  if (strcmp(action, "alternate") == 0) {
    const int period = doc["period"] | static_cast<int>(lights.alternatePeriodMs());
    const int level = doc["level"] | static_cast<int>(lights.alternateLevel());
    if (period < 0 || period > 5000 || level < 0 || level > 100) {
      sendJson(400, "{\"error\":\"invalid alternate settings\"}");
      return;
    }
    lights.setAlternatePeriodMs(static_cast<uint32_t>(period));
    lights.setAlternateLevel(static_cast<uint8_t>(level));
    lights.setAlternateEnabled(doc["enabled"] | false);
    sendJson(200, "{\"ok\":true}");
    return;
  }

  const char* zoneName = doc["zone"] | "both";
  const int value = doc["value"] | 0;
  uint32_t duration = doc["duration"] | 0;
  const bool isLevel = strcmp(action, "level") == 0;
  const bool isFade = strcmp(action, "fade") == 0;
  const bool validAction = isLevel || isFade ||
      strcmp(action, "on") == 0 ||
      strcmp(action, "off") == 0 ||
      strcmp(action, "toggle") == 0;
  if (!validAction) {
    sendJson(400, "{\"error\":\"unknown action\"}");
    return;
  }
  if ((isLevel || isFade) && (value < 0 || value > 100)) {
    sendJson(400, "{\"error\":\"invalid level\"}");
    return;
  }
  if (duration > 60000) {
    sendJson(400, "{\"error\":\"invalid duration\"}");
    return;
  }
  if (duration == 0) {
    const LightSettings& cfg = lights.settings();
    const int fadeValue = doc["value"] | 0;
    duration = (fadeValue == 0) ? cfg.fadeOffMs : cfg.fadeOnMs;
  }

  const bool bothZones = strcmp(zoneName, "both") == 0;
  const LightZoneId zoneId = bothZones ? ZONE_COUNT : zoneFromName(zoneName);
  if (!bothZones && zoneId >= ZONE_COUNT) {
    sendJson(400, "{\"error\":\"unknown zone\"}");
    return;
  }

  lights.cancelAutomationForManualControl();

  auto applyToZone = [&](LightZoneId id) {
    if (strcmp(action, "level") == 0) {
      lights.setLightLevel(id, static_cast<uint8_t>(value));
    } else if (strcmp(action, "on") == 0) {
      lights.setLightOn(id);
    } else if (strcmp(action, "off") == 0) {
      lights.setLightOff(id);
    } else if (strcmp(action, "fade") == 0) {
      lights.fadeLight(id, static_cast<uint8_t>(value), duration);
    } else if (strcmp(action, "toggle") == 0) {
      lights.toggleLight(id);
    }
  };

  if (bothZones) {
    if (strcmp(action, "on") == 0) {
      lights.allLightsOn();
    } else if (strcmp(action, "off") == 0) {
      lights.allLightsOff();
    } else if (strcmp(action, "level") == 0) {
      lights.setBothLights(static_cast<uint8_t>(value));
    } else {
      applyToZone(ZONE_CUNA);
      applyToZone(ZONE_SETUP);
    }
  } else {
    applyToZone(zoneId);
  }

  sendJson(200, "{\"ok\":true}");
}

void handleOtaStatus() {
  JsonDocument doc;
  remoteUpdate.statusJson(doc.to<JsonObject>());
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

void handleOtaCheck() {
  if (WiFi.status() != WL_CONNECTED) {
    sendJson(503, "{\"error\":\"sin WiFi\"}");
    return;
  }
  if (!remoteUpdate.requestCheck()) {
    sendJson(409, "{\"error\":\"ya hay una revision en curso\"}");
    return;
  }
  sendJson(202, "{\"ok\":true}");
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Conectando a %s", WIFI_SSID);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi: timeout.");
    return;
  }

  deviceIp = WiFi.localIP().toString();
  Serial.println("WiFi OK");
  Serial.print("Panel web: http://");
  Serial.println(deviceIp);
}

void setupOta() {
  ArduinoOTA.setHostname(MDNS_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    lights.onFirmwareUpdateStart();
    Serial.println("OTA: inicio");
  });
  ArduinoOTA.onEnd([]() { Serial.println("\nOTA: listo"); });
  ArduinoOTA.onProgress([](unsigned int prog, unsigned int total) {
    Serial.printf("OTA: %u%%\r", (prog / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t err) {
    Serial.printf("OTA error %u\n", err);
  });

  ArduinoOTA.begin();
  Serial.println("OTA listo (misma red WiFi, Arduino IDE → puerto de red).");
  Serial.print("Hostname OTA: ");
  Serial.println(MDNS_HOSTNAME);
}

void setup() {
  Serial.begin(115200);
  lights.begin();

  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    lights.startClock();
    if (MDNS.begin(MDNS_HOSTNAME)) {
      MDNS.addService("http", "tcp", 80);
    }
    setupOta();
  }

  server.on("/", HTTP_GET, handleIndexPage);
  server.on("/index.html", HTTP_GET, handleIndexPage);
  server.on("/api/state", HTTP_GET, handleState);
  server.on("/api/state", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/command", HTTP_POST, handleCommand);
  server.on("/api/command", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/schedule", HTTP_POST, handleSchedule);
  server.on("/api/schedule", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/routines", HTTP_POST, handleRoutines);
  server.on("/api/routines", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/settings", HTTP_POST, handleSettings);
  server.on("/api/settings", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/alternate", HTTP_POST, handleAlternate);
  server.on("/api/alternate", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/ota/status", HTTP_GET, handleOtaStatus);
  server.on("/api/ota/status", HTTP_OPTIONS, handleApiOptions);
  server.on("/api/ota/check", HTTP_POST, handleOtaCheck);
  server.on("/api/ota/check", HTTP_OPTIONS, handleApiOptions);
  server.begin();
  Serial.println("Servidor HTTP :80");

  remoteUpdate.begin(FW_VERSION, OTA_MANIFEST_URL, OTA_CHECK_INTERVAL_MS, OTA_BOOT_DELAY_MS, OTA_RETRY_MS);
}

void loop() {
  lights.update();
  server.handleClient();
  ArduinoOTA.handle();
  remoteUpdate.loop();
  if (remoteUpdate.flashing()) {
    lights.cancelAutomationIfFlashing();
  }
}
