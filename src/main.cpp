#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "settings.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "tempo.h"
#include "shelly.h"
#include "f1atb.h"
#include "zendure.h"
#include "controller.h"
#include "history.h"
#include "weather.h"
#include "app_state.h"

static uint32_t lastShellyPoll = 0;
static uint32_t lastZendurePoll = 0;
static uint32_t lastTempoPoll = 0;
static uint32_t lastF1atbPoll = 0;
static uint32_t lastControlEval = 0;
static uint32_t lastMdnsDiscovery = 0;
static uint32_t lastHistorySample = 0;
static uint32_t lastWeatherPoll = 0;

static bool waitForClockSync(uint32_t timeoutMs = 12000) {
  Serial.printf("[NTP] Attente synchronisation (max %lu ms)...\n", (unsigned long)timeoutMs);
  const uint32_t start = millis();
  struct tm t;

  while (millis() - start < timeoutMs) {
    if (localTimeSafe(t)) {
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
      Serial.printf("[NTP] OK heure locale=%s\n", buf);
      return true;
    }
    delay(200);
  }

  Serial.println("[NTP] ECHEC/timeout: Tempo sera retente automatiquement dans la boucle");
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  loadConfig();

  const bool wifiOk = connectWifi();

  if (!wifiOk) {
    startSetupPortal();
  } else {
    configTzTime(
      "CET-1CEST,M3.5.0/2,M10.5.0/3",
      "pool.ntp.org",
      "time.cloudflare.com"
    );

    startMdns();
  }

  setupWebServer();

  if (wifiOk) {
    Serial.println("[BOOT] Reseau disponible -> initialisation des services en ligne");

    // Tempo a besoin d'une vraie date locale pour calculer la journee 06:00 -> 06:00.
    // Ne plus lancer la premiere requete juste apres configTzTime(): SNTP est asynchrone.
    const bool clockReady = waitForClockSync();
    if (clockReady) {
      refreshTempo();
      lastTempoPoll = millis();
    } else {
      // Laisser lastTempoPoll a 0 afin que la boucle retente des que l'horloge devient valide.
      lastTempoPoll = 0;
    }

    pollWeather();
    lastWeatherPoll = millis();
    discoverZendure();
    lastMdnsDiscovery = millis();
    pollShelly();
    if (cfg.f1atbEnabled) {
      pollF1ATB();
      lastF1atbPoll = millis();
    }
    pollZendures();

    // Fail-safe: never restore a manual override after reboot.
    // Force one real reconciliation with the AUTO decision so a Zendure left
    // in forced charge before the reboot cannot remain in a stale mode.
    requestedMode = ControlMode::AUTO;
    manualUntilEpoch = 0;
    manualUntilMs = 0;
    evaluateControl(true);
  }
}

void loop() {
  handleWebServer();

  if (isSetupPortal()) {
    processSetupDns();
    delay(2);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    delay(50);
    return;
  }

  const uint32_t now = millis();

  // Si le NTP n'etait pas encore pret au boot, retenter Tempo rapidement des que l'heure apparait.
  // 5 s minimum entre essais pour ne pas marteler RTE.
  struct tm timeCheck;
  if (tempoLastOkMs == 0 && localTimeSafe(timeCheck) && (lastTempoPoll == 0 || now - lastTempoPoll >= 5000)) {
    lastTempoPoll = now;
    refreshTempo();
  }

  if (now - lastShellyPoll >= SHELLY_POLL_MS) {
    lastShellyPoll = now;
    pollShelly();
  }

  if (now - lastZendurePoll >= ZENDURE_STATUS_POLL_MS) {
    lastZendurePoll = now;
    pollZendures();

    // Close the loop: compare freshly read real properties to the desired
    // controller state immediately instead of waiting for a later transition.
    evaluateControl();
  }

  if (cfg.f1atbEnabled && now - lastF1atbPoll >= F1ATB_POLL_MS) {
    lastF1atbPoll = now;
    pollF1ATB();
  }

  if (now - lastTempoPoll >= TEMPO_REFRESH_MS) {
    lastTempoPoll = now;
    refreshTempo();
  }

  if (now - lastWeatherPoll >= WEATHER_REFRESH_MS) {
    lastWeatherPoll = now;
    pollWeather();
  }

  if (now - lastMdnsDiscovery >= MDNS_DISCOVERY_MS) {
    lastMdnsDiscovery = now;
    discoverZendure();
  }

  if (now - lastControlEval >= CONTROL_EVAL_MS) {
    lastControlEval = now;
    evaluateControl();
  }

  if (now - lastHistorySample >= HISTORY_SAMPLE_MS) {
    lastHistorySample = now;
    addHistory();
  }

  delay(2);
}
