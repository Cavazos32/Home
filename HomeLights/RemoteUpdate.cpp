#include "RemoteUpdate.h"

#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Update.h>

#include "GitHubRootCA.h"

RemoteUpdate remoteUpdate;

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

static constexpr uint32_t kTaskStack = 16384;
static constexpr uint32_t kHttpTimeoutMs = 15000;
static constexpr size_t kMaxManifestBytes = 1024;

// Seguridad TLS: validamos el certificado del servidor contra las CA raíz de
// GitHub embebidas en GitHubRootCA.h (no se usa setInsecure()). Así nadie en
// medio de la red puede servir un firmware falso. Además el .bin se verifica
// con el MD5 y SHA-256 del manifiesto antes de marcarlo como arrancable.
static void configureTls(NetworkClientSecure& client) {
  client.setCACert(GITHUB_ROOT_CA);
  client.setTimeout(kHttpTimeoutMs / 1000);
}

void RemoteUpdate::begin(const char* currentVersion, const char* manifestUrl, uint32_t intervalMs, uint32_t bootDelayMs, uint32_t retryMs) {
  _currentVersion = currentVersion;
  _manifestUrl = manifestUrl;
  _intervalMs = intervalMs;
  _bootDelayMs = bootDelayMs;
  _retryMs = retryMs;
  _nextCheckMs = millis() + bootDelayMs;
  setStatus("idle", "Esperando primera revision");
  Serial.printf("Firmware v%s (OTA por descarga: %s)\n", currentVersion, manifestUrl);
}

bool RemoteUpdate::requestCheck() {
  if (_busy) return false;
  _forceRequested = true;
  return true;
}

void RemoteUpdate::loop() {
  if (_busy) return;
  if (WiFi.status() != WL_CONNECTED) return;

  const uint32_t now = millis();
  const bool due = static_cast<int32_t>(now - _nextCheckMs) >= 0;
  if (!due && !_forceRequested) return;

  // No pisar una carga por ArduinoOTA (LAN) que esté en curso.
  if (Update.isRunning()) return;

  _forceRequested = false;
  _busy = true;
  _progress = 0;
  if (xTaskCreatePinnedToCore(taskEntry, "remote_ota", kTaskStack, this, 1, nullptr, 0) != pdPASS) {
    _busy = false;
    setStatus("error", "No se pudo crear la tarea OTA");
    _nextCheckMs = now + _retryMs;
  }
}

void RemoteUpdate::taskEntry(void* arg) {
  auto* self = static_cast<RemoteUpdate*>(arg);
  self->runCheck();
  self->_busy = false;
  vTaskDelete(nullptr);
}

void RemoteUpdate::setStatus(const char* state, const char* message) {
  portENTER_CRITICAL(&s_mux);
  strlcpy(_state, state, sizeof(_state));
  strlcpy(_message, message ? message : "", sizeof(_message));
  portEXIT_CRITICAL(&s_mux);
  Serial.printf("OTA web [%s] %s\n", state, message ? message : "");
}

int RemoteUpdate::compareVersions(const char* a, const char* b) {
  if (!a) a = "";
  if (!b) b = "";
  if (*a == 'v' || *a == 'V') a++;
  if (*b == 'v' || *b == 'V') b++;
  for (int i = 0; i < 4; i++) {
    const long na = strtol(a, const_cast<char**>(&a), 10);
    const long nb = strtol(b, const_cast<char**>(&b), 10);
    if (na != nb) return na < nb ? -1 : 1;
    if (*a == '.') a++;
    if (*b == '.') b++;
    if (!*a && !*b) break;
  }
  return 0;
}

bool RemoteUpdate::fetchManifest(String& version, String& url, String& md5, String& sha256) {
  NetworkClientSecure client;
  configureTls(client);

  HTTPClient http;
  http.useHTTP10(true);  // sin keep-alive: GitHub redirige a otro host
  http.setTimeout(kHttpTimeoutMs);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setUserAgent("HomeLights-ESP32");
  if (!http.begin(client, _manifestUrl)) {
    setStatus("error", "URL de manifiesto invalida");
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    char msg[96];
    if (code == HTTP_CODE_NOT_FOUND) {
      snprintf(msg, sizeof(msg), "Manifiesto no encontrado (404): no hay release publicado");
    } else {
      snprintf(msg, sizeof(msg), "Manifiesto HTTP %d (%s)", code, http.errorToString(code).c_str());
    }
    http.end();
    setStatus("error", msg);
    return false;
  }

  const int len = http.getSize();
  if (len > static_cast<int>(kMaxManifestBytes)) {
    http.end();
    setStatus("error", "Manifiesto demasiado grande");
    return false;
  }
  const String body = http.getString();
  http.end();

  JsonDocument doc;
  if (body.length() > kMaxManifestBytes || deserializeJson(doc, body)) {
    setStatus("error", "Manifiesto con JSON invalido");
    return false;
  }

  version = doc["version"] | "";
  url = doc["url"] | "";
  md5 = doc["md5"] | "";
  sha256 = doc["sha256"] | "";

  if (version.isEmpty() || !url.startsWith("https://")) {
    setStatus("error", "Manifiesto sin version o url https");
    return false;
  }
  if (md5.length() != 32 && sha256.length() != 64) {
    // Sin checksum no flasheamos: una descarga corrupta podría pasar.
    setStatus("error", "Manifiesto sin md5/sha256");
    return false;
  }
  return true;
}

void RemoteUpdate::runCheck() {
  setStatus("checking", "Buscando version nueva...");

  String version, url, md5, sha256;
  const bool ok = fetchManifest(version, url, md5, sha256);

  portENTER_CRITICAL(&s_mux);
  _lastCheckMs = millis();
  _checkedOnce = true;
  if (ok) strlcpy(_latest, version.c_str(), sizeof(_latest));
  portEXIT_CRITICAL(&s_mux);

  if (!ok) {
    _nextCheckMs = millis() + _retryMs;
    return;
  }

  if (compareVersions(version.c_str(), _currentVersion) <= 0) {
    char msg[96];
    snprintf(msg, sizeof(msg), "Al dia (v%s; publicado v%s)", _currentVersion, version.c_str());
    setStatus("up_to_date", msg);
    _nextCheckMs = millis() + _intervalMs;
    return;
  }

  if (Update.isRunning()) {
    setStatus("error", "Otra actualizacion en curso");
    _nextCheckMs = millis() + _retryMs;
    return;
  }

  char msg[96];
  snprintf(msg, sizeof(msg), "Descargando v%s...", version.c_str());
  setStatus("downloading", msg);

  NetworkClientSecure client;
  configureTls(client);

  HTTPUpdate updater(static_cast<int>(kHttpTimeoutMs));
  updater.rebootOnUpdate(false);
  updater.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  // Update.end() rechaza la imagen si el MD5/SHA-256 no coincide; el firmware
  // actual sigue intacto y no se cambia la partición de arranque.
  if (md5.length() == 32) updater.setMD5sum(md5);
  if (sha256.length() == 64) updater.setSHA256sum(sha256);
  updater.onStart([this]() { _flashing = true; });
  updater.onProgress([this](int cur, int total) {
    if (total > 0) _progress = static_cast<int>((static_cast<int64_t>(cur) * 100) / total);
  });

  const HTTPUpdateResult res = updater.update(client, url, _currentVersion);
  _flashing = false;

  switch (res) {
    case HTTP_UPDATE_OK:
      snprintf(msg, sizeof(msg), "Actualizado a v%s, reiniciando...", version.c_str());
      setStatus("updated", msg);
      delay(1500);  // deja responder a /api/ota/status antes de reiniciar
      ESP.restart();
      break;
    case HTTP_UPDATE_NO_UPDATES:
      setStatus("up_to_date", "El servidor no envio firmware (304)");
      _nextCheckMs = millis() + _intervalMs;
      break;
    case HTTP_UPDATE_FAILED:
    default:
      snprintf(msg, sizeof(msg), "Fallo (%d): %s", updater.getLastError(), updater.getLastErrorString().c_str());
      setStatus("error", msg);
      _nextCheckMs = millis() + _retryMs;
      break;
  }
}

void RemoteUpdate::statusJson(JsonObject obj) {
  char state[sizeof(_state)];
  char message[sizeof(_message)];
  char latest[sizeof(_latest)];
  uint32_t lastCheckMs;
  bool checkedOnce;
  portENTER_CRITICAL(&s_mux);
  memcpy(state, _state, sizeof(state));
  memcpy(message, _message, sizeof(message));
  memcpy(latest, _latest, sizeof(latest));
  lastCheckMs = _lastCheckMs;
  checkedOnce = _checkedOnce;
  portEXIT_CRITICAL(&s_mux);

  const uint32_t now = millis();
  obj["version"] = _currentVersion;
  obj["latest"] = latest;
  obj["state"] = state;
  obj["message"] = message;
  obj["busy"] = static_cast<bool>(_busy);
  obj["progress"] = static_cast<int>(_progress);
  obj["lastCheckAgoS"] = checkedOnce ? static_cast<int32_t>((now - lastCheckMs) / 1000) : -1;
  const int32_t untilNext = static_cast<int32_t>(_nextCheckMs - now);
  obj["nextCheckInS"] = (_busy || untilNext < 0) ? 0 : untilNext / 1000;
  obj["manifest"] = _manifestUrl;
}
