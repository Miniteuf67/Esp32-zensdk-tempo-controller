#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

#include "tempo.h"
#include "app_state.h"
#include "settings.h"
#include "config.h"
#include "http_helpers.h"

enum class DayFetchState : uint8_t {
  FAILED = 0,
  UNDEFINED = 1,
  DEFINED = 2
};

static TempoColor parseTempoColor(const String& value) {
  String v = value;
  v.toUpperCase();

  if (v == "BLUE" || v.indexOf("BLEU") >= 0) return TempoColor::BLUE;
  if (v == "WHITE" || v.indexOf("BLANC") >= 0) return TempoColor::WHITE;
  if (v == "RED" || v.indexOf("ROUGE") >= 0) return TempoColor::RED;

  return TempoColor::UNKNOWN;
}

static String formatLocalDate(time_t ts) {
  struct tm t;
  localtime_r(&ts, &t);
  char buf[11];
  strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
  return String(buf);
}

static bool expectedTempoDates(String& currentDate, String& nextDate) {
  struct tm nowTm;
  if (!localTimeSafe(nowTm)) return false;

  time_t now;
  time(&now);

  // Même logique fonctionnelle que F1ATB : la journée Tempo va de 06:00 à 06:00.
  // En retirant 6 h, la date civile obtenue est la date de début de la journée Tempo courante.
  const time_t tempoDay = now - (6 * 60 * 60);
  currentDate = formatLocalDate(tempoDay);
  nextDate = formatLocalDate(tempoDay + (24 * 60 * 60));
  return true;
}

static DayFetchState readRteDay(
  JsonObject values,
  const String& date,
  TempoColor& color
) {
  if (date.isEmpty()) return DayFetchState::FAILED;

  JsonVariant v = values[date.c_str()];
  if (v.isNull()) {
    color = TempoColor::UNKNOWN;
    return DayFetchState::UNDEFINED;
  }

  color = parseTempoColor(String(v.as<const char*>()));
  return color == TempoColor::UNKNOWN
    ? DayFetchState::UNDEFINED
    : DayFetchState::DEFINED;
}

static void updateCurrentTariffFromTempo() {
  struct tm nowTm;
  if (!localTimeSafe(nowTm)) {
    tempoNowColor = tempoToday;
    tempoNowTariff = "";
    tempoNowScheduleCode = -1;
    return;
  }

  const bool hp = nowTm.tm_hour >= 6 && nowTm.tm_hour < 22;
  tempoNowColor = tempoToday;
  tempoNowScheduleCode = hp ? 1 : 2;

  if (tempoToday == TempoColor::UNKNOWN) {
    tempoNowTariff = hp ? "HP" : "HC";
  } else {
    tempoNowTariff = String("Tempo ") + tempoColorName(tempoToday) + (hp ? " HP" : " HC");
  }
  tempoNowLastOkMs = millis();
}

void refreshTempo() {
  Serial.println("[TEMPO] Actualisation RTE (methode F1ATB)...");

  String expectedToday;
  String expectedTomorrow;
  if (!expectedTempoDates(expectedToday, expectedTomorrow)) {
    Serial.println("[TEMPO] Date locale indisponible, conservation du cache");
    return;
  }

  JsonDocument doc;
  const String url = String("https://") + TEMPO_HOST + TEMPO_PATH;

  if (!httpGetJson(url, doc, 8000)) {
    Serial.println("[TEMPO] RTE inaccessible, conservation du cache");
    updateCurrentTariffFromTempo();
    return;
  }

  JsonObject values = doc["values"].as<JsonObject>();
  if (values.isNull()) {
    Serial.println("[TEMPO] JSON RTE sans objet 'values', conservation du cache");
    updateCurrentTariffFromTempo();
    return;
  }

  TempoColor today = TempoColor::UNKNOWN;
  TempoColor tomorrow = TempoColor::UNKNOWN;
  const DayFetchState todayState = readRteDay(values, expectedToday, today);
  const DayFetchState tomorrowState = readRteDay(values, expectedTomorrow, tomorrow);

  // Ne jamais détruire une valeur connue sur simple absence de clé.
  // C'est particulièrement important entre 00:00 et 05:59 : RTE peut ne plus renvoyer J-1,
  // alors que cette couleur reste la journée Tempo courante jusqu'à 06:00.
  if (todayState == DayFetchState::DEFINED) {
    tempoToday = today;
    tempoTodayDate = expectedToday;
  } else if (tempoTodayDate != expectedToday) {
    tempoToday = TempoColor::UNKNOWN;
    tempoTodayDate = "";
  }

  if (tomorrowState == DayFetchState::DEFINED) {
    tempoTomorrow = tomorrow;
    tempoTomorrowDate = expectedTomorrow;
  } else {
    // Si RTE n'a pas encore publié J+1, on affiche explicitement "Non défini".
    // On ne conserve pas une ancienne prévision d'une autre date.
    if (tempoTomorrowDate != expectedTomorrow) {
      tempoTomorrow = TempoColor::UNKNOWN;
      tempoTomorrowDate = "";
    }
  }

  tempoLastOkMs = millis();
  cacheTempo();
  updateCurrentTariffFromTempo();

  Serial.printf("[TEMPO] RTE dates attendues J=%s J+1=%s | J=%s (%u) | J+1=%s (%u)\n",
                expectedToday.c_str(), expectedTomorrow.c_str(),
                tempoColorName(tempoToday), static_cast<unsigned>(todayState),
                tempoColorName(tempoTomorrow), static_cast<unsigned>(tomorrowState));
}

bool tempoTomorrowFresh() {
  if (
    tempoTomorrow == TempoColor::UNKNOWN ||
    tempoTomorrowDate.isEmpty()
  ) {
    return false;
  }

  String expectedToday;
  String expectedTomorrow;
  if (!expectedTempoDates(expectedToday, expectedTomorrow)) return false;

  return tempoTomorrowDate == expectedTomorrow;
}
