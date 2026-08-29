#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "weather.h"
#include "config.h"
#include "app_state.h"
#include "http_helpers.h"

static String urlEncode(const String& input) {
  String out;
  char buf[4];

  for (size_t i = 0; i < input.length(); ++i) {
    const unsigned char c = static_cast<unsigned char>(input[i]);

    if (
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '-' || c == '_' || c == '.' || c == '~'
    ) {
      out += static_cast<char>(c);
    } else if (c == ' ') {
      out += "%20";
    } else {
      snprintf(buf, sizeof(buf), "%%%02X", c);
      out += buf;
    }
  }

  return out;
}

bool searchWeatherCity(const String& query, JsonDocument& out) {
  if (query.length() < 2) return false;

  const String url =
    String("https://") + GEOCODING_HOST +
    "/v1/search?name=" + urlEncode(query) +
    "&count=8&language=fr&format=json";

  return httpGetJson(url, out, 5000);
}

void pollWeather() {
  Serial.println("[METEO] Actualisation...");
  if (
    !cfg.weatherEnabled ||
    cfg.weatherLat < -90.0 || cfg.weatherLat > 90.0 ||
    cfg.weatherLon < -180.0 || cfg.weatherLon > 180.0 ||
    (cfg.weatherLat == 0.0 && cfg.weatherLon == 0.0)
  ) {
    weatherOnline = false;
    Serial.printf("[METEO] Ignoree: enabled=%d lat=%.6f lon=%.6f\n",
                  cfg.weatherEnabled, cfg.weatherLat, cfg.weatherLon);
    return;
  }

  const String url =
    String("https://") + WEATHER_HOST +
    "/v1/forecast?latitude=" + String(cfg.weatherLat, 6) +
    "&longitude=" + String(cfg.weatherLon, 6) +
    "&current=temperature_2m,apparent_temperature,relative_humidity_2m,precipitation,weather_code,wind_speed_10m" +
    "&daily=weather_code,temperature_2m_max,temperature_2m_min" +
    "&forecast_days=2&wind_speed_unit=kmh&timezone=auto";

  JsonDocument doc;

  if (!httpGetJson(url, doc, 5000)) {
    weatherOnline = false;
    Serial.println("[METEO] Requete ECHEC");
    return;
  }

  JsonObject current = doc["current"];

  if (current.isNull()) {
    weatherOnline = false;
    return;
  }

  weatherTempC = current["temperature_2m"] | NAN;
  weatherFeelsC = current["apparent_temperature"] | NAN;
  weatherHumidity = current["relative_humidity_2m"] | NAN;
  weatherPrecipMm = current["precipitation"] | NAN;
  weatherCode = current["weather_code"] | -1;
  weatherWindKmh = current["wind_speed_10m"] | NAN;

  JsonObject daily = doc["daily"];
  if (!daily.isNull()) {
    JsonArray codes = daily["weather_code"].as<JsonArray>();
    JsonArray mins = daily["temperature_2m_min"].as<JsonArray>();
    JsonArray maxs = daily["temperature_2m_max"].as<JsonArray>();

    if (codes.size() > 1) weatherTomorrowCode = codes[1] | -1;
    if (mins.size() > 1) weatherTomorrowMinC = mins[1] | NAN;
    if (maxs.size() > 1) weatherTomorrowMaxC = maxs[1] | NAN;
  }

  weatherOnline = !isnan(weatherTempC);
  if (weatherOnline) {
    weatherLastOkMs = millis();
    Serial.printf("[METEO] OK %s: %.1f C, humidite %.0f %%\n",
                  cfg.weatherDisplayName.c_str(), weatherTempC, weatherHumidity);
  } else {
    Serial.println("[METEO] Reponse recue mais temperature invalide");
  }
}
