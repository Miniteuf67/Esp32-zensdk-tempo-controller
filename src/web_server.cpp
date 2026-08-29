#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <Preferences.h>
#include <Update.h>

#include "web_server.h"
#include "web_pages.h"
#include "wifi_manager.h"
#include "app_state.h"
#include "settings.h"
#include "controller.h"
#include "zendure.h"
#include "config.h"
#include "weather.h"
#include "f1atb.h"

static WebServer server(80);
static String sessionToken;

static String randomToken() {
  char buf[33];

  for (int i = 0; i < 16; ++i) {
    const uint8_t b = esp_random() & 0xff;
    snprintf(buf + i * 2, 3, "%02x", b);
  }

  return String(buf);
}

static bool isAdmin() {
  if (sessionToken.isEmpty()) return false;
  if (!server.hasHeader("Cookie")) return false;

  const String cookie = server.header("Cookie");
  return cookie.indexOf("zt_session=" + sessionToken) >= 0;
}

static void requireAdminOr401() {
  server.send(401, "application/json", R"({"ok":false,"error":"auth_required"})");
}

static bool parseBody(JsonDocument& doc) {
  if (!server.hasArg("plain")) return false;
  return !deserializeJson(doc, server.arg("plain"));
}

static void sendStatus() {
  JsonDocument d;

  d["wifi"]["ssid"] = WiFi.SSID();
  d["wifi"]["ip"] = WiFi.localIP().toString();
  d["wifi"]["rssi"] = WiFi.RSSI();
  d["system"]["free_heap"] = ESP.getFreeHeap();
  d["system"]["min_free_heap"] = ESP.getMinFreeHeap();
  d["system"]["heap_size"] = ESP.getHeapSize();
  d["system"]["max_alloc_heap"] = ESP.getMaxAllocHeap();
  d["system"]["flash_size"] = ESP.getFlashChipSize();
  d["system"]["sketch_size"] = ESP.getSketchSize();
  d["system"]["free_sketch_space"] = ESP.getFreeSketchSpace();
  d["system"]["uptime_s"] = millis() / 1000UL;

  d["tempo"]["today"] = tempoColorName(tempoToday);
  d["tempo"]["tomorrow"] = tempoColorName(tempoTomorrow);
  d["tempo"]["today_date"] = tempoTodayDate;
  d["tempo"]["tomorrow_date"] = tempoTomorrowDate;
  d["tempo"]["tomorrow_defined"] = tempoTomorrow != TempoColor::UNKNOWN && !tempoTomorrowDate.isEmpty();

  struct tm t;
  String period = "--";

  if (localTimeSafe(t)) {
    period = (t.tm_hour >= 22 || t.tm_hour < 6) ? "HC" : "HP";
  }

  d["tempo"]["period"] = period;
  d["tempo"]["window"] = tempoWindowName();
  d["tempo"]["precharge_target"] = tempoColorName(prechargeTargetColor());
  d["tempo"]["precharge_target_date"] = tempoTomorrowDate;
  d["tempo"]["precharge_active"] = shouldAutoForceCharge();
  d["tempo"]["now_color"] = tempoColorName(tempoNowColor);
  d["tempo"]["now_tariff"] = tempoNowTariff;
  d["tempo"]["now_schedule_code"] = tempoNowScheduleCode;

  d["shelly"]["online"] = (millis() - shellyLastOkMs) < 10000;
  d["shelly"]["power_w"] = isnan(shellyPowerW) ? 0 : shellyPowerW;
  d["shelly"]["voltage_v"] = isnan(shellyVoltageV) ? 0 : shellyVoltageV;
  d["shelly"]["current_a"] = isnan(shellyCurrentA) ? 0 : shellyCurrentA;
  d["shelly"]["pf"] = isnan(shellyPf) ? 0 : shellyPf;
  d["shelly"]["rssi"] = shellyRssi;
  d["shelly"]["profile"] = cfg.shellyMonophase ? "mono" : "tri";
  d["shelly"]["grid_channel"] = cfg.shellyGridChannel;
  d["shelly"]["routed_enabled"] = cfg.shellyMonophase && cfg.shellyRoutedChannel >= 0;
  d["shelly"]["routed_channel"] = cfg.shellyRoutedChannel;
  d["shelly"]["routed_online"] =
    cfg.shellyMonophase && cfg.shellyRoutedChannel >= 0 &&
    (millis() - shellyRoutedLastOkMs) < 10000;
  d["shelly"]["routed_power_w"] = isnan(shellyRoutedPowerW) ? 0 : shellyRoutedPowerW;
  d["shelly"]["routed_energy_wh"] = isnan(shellyRoutedEnergyWh) ? 0 : shellyRoutedEnergyWh;
  d["shelly"]["routed_total_wh"] = isnan(shellyRoutedTotalWh) ? 0 : shellyRoutedTotalWh;
  d["shelly"]["routed_today_wh"] = isnan(shellyRoutedTodayWh) ? 0 : shellyRoutedTodayWh;
  d["shelly"]["grid_import_total_wh"] = isnan(shellyGridImportTotalWh) ? 0 : shellyGridImportTotalWh;
  d["shelly"]["grid_export_total_wh"] = isnan(shellyGridExportTotalWh) ? 0 : shellyGridExportTotalWh;
  d["shelly"]["grid_import_today_wh"] = isnan(shellyGridImportTodayWh) ? 0 : shellyGridImportTodayWh;
  d["shelly"]["grid_export_today_wh"] = isnan(shellyGridExportTodayWh) ? 0 : shellyGridExportTodayWh;
  d["shelly"]["aux_enabled"] = cfg.shellyMonophase && cfg.shellyAuxChannel >= 0;
  d["shelly"]["aux_channel"] = cfg.shellyAuxChannel;
  d["shelly"]["aux_label"] = cfg.shellyAuxLabel;
  d["shelly"]["aux_online"] = cfg.shellyMonophase && cfg.shellyAuxChannel >= 0 && (millis() - shellyAuxLastOkMs) < 10000;
  d["shelly"]["aux_power_w"] = isnan(shellyAuxPowerW) ? 0 : shellyAuxPowerW;
  d["shelly"]["aux_total_wh"] = isnan(shellyAuxTotalWh) ? 0 : shellyAuxTotalWh;
  d["shelly"]["aux_today_wh"] = isnan(shellyAuxTodayWh) ? 0 : shellyAuxTodayWh;
  d["shelly"]["energy_day"] = shellyEnergyDayDate;
  d["energy"]["pv_today_wh"] = pvEnergyTodayWh;
  d["energy"]["pv_total_wh"] = pvEnergyTotalWh;
  d["energy"]["pv_day"] = pvEnergyDayDate;

  int32_t totalPvW = 0;
  int32_t totalHomeW = 0;
  int32_t totalBatteryDischargeW = 0;
  int32_t totalBatteryChargeW = 0;

  for (size_t i = 0; i < zendureCount; ++i) {
    if (!zendures[i].configured || !zendures[i].enabled || !zendures[i].online) continue;
    totalPvW += max(0, zendures[i].solarInputPower);
    totalHomeW += max(0, zendures[i].outputHomePower);
    totalBatteryDischargeW += max(0, zendures[i].packInputPower);
    totalBatteryChargeW += max(0, zendures[i].outputPackPower);
  }

  const int32_t gridNetW = isnan(shellyPowerW) ? 0 : lroundf(shellyPowerW);
  const int32_t routedW =
    (cfg.shellyMonophase && cfg.shellyRoutedChannel >= 0 && !isnan(shellyRoutedPowerW))
      ? max<int32_t>(0, lroundf(shellyRoutedPowerW))
      : 0;

  const int32_t houseW = max<int32_t>(0, gridNetW + totalHomeW);
  const int32_t batteryW = totalBatteryDischargeW - totalBatteryChargeW;

  d["flows"]["pv_w"] = totalPvW;
  d["flows"]["house_w"] = houseW;
  d["flows"]["routed_w"] = routedW;
  d["flows"]["battery_w"] = batteryW;
  d["flows"]["from_grid_w"] = max<int32_t>(0, gridNetW);

  d["f1atb"]["enabled"] = cfg.f1atbEnabled;
  d["f1atb"]["tempo"] = cfg.f1atbEnabled ? f1atbTempo : "";
  d["f1atb"]["action_label"] = cfg.f1atbActionLabel;
  d["f1atb"]["action_number"] = cfg.f1atbActionNumber;
  d["f1atb"]["detected_name"] = f1atbActionDetectedName;
  d["f1atb"]["action_state"] = f1atbActionState;
  d["f1atb"]["action_online"] = f1atbActionOnline;
  d["f1atb"]["force_minutes"] = f1atbActionForceMinutes;
  const bool f1atbFresh = f1atbLastOkMs != 0 && (millis() - f1atbLastOkMs) < (F1ATB_POLL_MS * 3UL);
  d["f1atb"]["online"] = cfg.f1atbEnabled && f1atbOnline && f1atbFresh;
  d["f1atb"]["synced"] = cfg.f1atbEnabled && f1atbActionOnline && f1atbFresh;
  d["f1atb"]["last_ok_age_s"] = f1atbLastOkMs ? ((millis() - f1atbLastOkMs) / 1000UL) : 0;
  d["weather"]["enabled"] = cfg.weatherEnabled;
  d["weather"]["online"] = weatherOnline;
  d["weather"]["city"] = cfg.weatherDisplayName;
  d["weather"]["temp_c"] = isnan(weatherTempC) ? 0 : weatherTempC;
  d["weather"]["feels_c"] = isnan(weatherFeelsC) ? 0 : weatherFeelsC;
  d["weather"]["humidity"] = isnan(weatherHumidity) ? 0 : weatherHumidity;
  d["weather"]["wind_kmh"] = isnan(weatherWindKmh) ? 0 : weatherWindKmh;
  d["weather"]["precip_mm"] = isnan(weatherPrecipMm) ? 0 : weatherPrecipMm;
  d["weather"]["code"] = weatherCode;
  d["weather"]["tomorrow_code"] = weatherTomorrowCode;
  d["weather"]["tomorrow_min_c"] = isnan(weatherTomorrowMinC) ? 0 : weatherTomorrowMinC;
  d["weather"]["tomorrow_max_c"] = isnan(weatherTomorrowMaxC) ? 0 : weatherTomorrowMaxC;


  d["auto_force"] = shouldAutoForceCharge();

  d["control"]["requested"] =
    requestedMode == ControlMode::AUTO
      ? "AUTO"
      : requestedMode == ControlMode::FORCE_CHARGE
        ? "Charge forcée"
        : "Autoconsommation";

  const ControlMode effective = effectiveMode();

  d["control"]["effective"] =
    effective == ControlMode::FORCE_CHARGE
      ? "Charge"
      : "Autoconsommation";

  d["control"]["manual_until"] = manualUntilEpoch;
  d["control"]["zendure_synced"] =
    zendureStateMatches(effective, cfg.totalChargeW);
  d["control"]["release_pending"] = hasZendureReleasePending();

  if (isAdmin()) {
    d["config"]["hostname"] = cfg.hostname;
    d["config"]["wifi_static_enabled"] = cfg.wifiStaticEnabled;
    d["config"]["wifi_static_ip"] = cfg.wifiStaticIp;
    d["config"]["wifi_gateway"] = cfg.wifiGateway;
    d["config"]["wifi_subnet"] = cfg.wifiSubnet;
    d["config"]["wifi_dns1"] = cfg.wifiDns1;
    d["config"]["wifi_dns2"] = cfg.wifiDns2;
    d["config"]["shelly_ip"] = cfg.shellyIp;
    d["config"]["shelly_profile"] = cfg.shellyMonophase ? "mono" : "tri";
    d["config"]["shelly_grid_channel"] = cfg.shellyGridChannel;
    d["config"]["shelly_routed_channel"] = cfg.shellyRoutedChannel;
    d["config"]["shelly_aux_channel"] = cfg.shellyAuxChannel;
    d["config"]["shelly_aux_label"] = cfg.shellyAuxLabel;
    d["config"]["f1atb_ip"] = cfg.f1atbIp;
    d["config"]["f1atb_enabled"] = cfg.f1atbEnabled;
    d["config"]["f1atb_action_label"] = cfg.f1atbActionLabel;
    d["config"]["f1atb_action_number"] = cfg.f1atbActionNumber;
    d["config"]["weather_enabled"] = cfg.weatherEnabled;
    d["config"]["weather_city"] = cfg.weatherCity;
    d["config"]["weather_display_name"] = cfg.weatherDisplayName;
    d["config"]["weather_lat"] = cfg.weatherLat;
    d["config"]["weather_lon"] = cfg.weatherLon;
    d["config"]["charge_w"] = cfg.totalChargeW;
    d["config"]["zendure_writes"] = cfg.zendureWritesEnabled;
  }

  JsonArray z = d["zendure"].to<JsonArray>();

  for (size_t i = 0; i < zendureCount; ++i) {
    JsonObject o = z.add<JsonObject>();

    o["configured"] = zendures[i].configured;
    o["enabled"] = zendures[i].enabled;
    o["label"] = zendures[i].label;

    // Sensitive configuration is returned only to an authenticated admin.
    // The public dashboard never receives serial numbers or manual IP settings.
    if (isAdmin()) {
      o["sn"] = zendures[i].sn;
      o["manual_ip"] = zendures[i].manualIp;
    }

    o["name"] = zendures[i].name;
    if (isAdmin()) {
      o["ip"] = zendures[i].ip;
    }
    o["online"] = zendures[i].online;
    o["soc"] = zendures[i].soc;
    o["grid_w"] = zendures[i].gridInputPower;
    o["solar_w"] = zendures[i].solarInputPower;
    o["home_w"] = zendures[i].outputHomePower;
    o["battery_discharge_w"] = zendures[i].packInputPower;
    o["battery_charge_w"] = zendures[i].outputPackPower;
    o["input_limit"] = zendures[i].inputLimit;
    o["output_limit"] = zendures[i].outputLimit;
    o["ac_mode"] = zendures[i].acMode;
    o["smart_mode"] = zendures[i].smartMode;
  }

  String out;
  serializeJson(d, out);
  server.send(200, "application/json", out);
}

static void sendHistory() {
  // Keep the full 30 s ring buffer in RAM, but do not send all 2880 samples
  // to the browser. A 24 h chart does not visually benefit from that density,
  // especially on a phone. Downsample to about 720 averaged buckets (2 min
  // at the normal 30 s cadence) and batch TCP writes to avoid thousands of
  // tiny sendContent() calls.
  constexpr size_t MAX_WEB_POINTS = 720;
  const size_t stride = historyCount > MAX_WEB_POINTS
    ? (historyCount + MAX_WEB_POINTS - 1) / MAX_WEB_POINTS
    : 1;

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");

  String chunk;
  chunk.reserve(4096);
  chunk = "{\"points\":[";

  const size_t start =
    (historyHead + HISTORY_POINTS - historyCount) % HISTORY_POINTS;

  bool first = true;
  char item[180];

  for (size_t i = 0; i < historyCount; i += stride) {
    const size_t bucketCount = min(stride, historyCount - i);
    int32_t pv = 0, house = 0, battery = 0, routed = 0, gridstore = 0;
    uint32_t epoch = 0;

    for (size_t j = 0; j < bucketCount; ++j) {
      const size_t index = (start + i + j) % HISTORY_POINTS;
      const HistoryPoint& h = historyBuf[index];
      pv += h.pvW;
      house += h.houseW;
      battery += h.batteryW;
      routed += h.routedW;
      gridstore += h.gridStoreW;
      epoch = h.epoch; // timestamp of the newest sample in the bucket
    }

    pv /= static_cast<int32_t>(bucketCount);
    house /= static_cast<int32_t>(bucketCount);
    battery /= static_cast<int32_t>(bucketCount);
    routed /= static_cast<int32_t>(bucketCount);
    gridstore /= static_cast<int32_t>(bucketCount);

    const int n = snprintf(
      item,
      sizeof(item),
      "%s{\"t\":%lu,\"pv\":%ld,\"house\":%ld,\"battery\":%ld,\"routed\":%ld,\"gridstore\":%ld}",
      first ? "" : ",",
      static_cast<unsigned long>(epoch),
      static_cast<long>(pv),
      static_cast<long>(house),
      static_cast<long>(battery),
      static_cast<long>(routed),
      static_cast<long>(gridstore)
    );

    first = false;
    if (n > 0) chunk += item;

    // Send in ~4 kB blocks instead of one TCP write per point.
    if (chunk.length() >= 3500) {
      server.sendContent(chunk);
      chunk = "";
      delay(0);
    }
  }

  chunk += "]}";
  if (chunk.length()) server.sendContent(chunk);
  server.sendContent("");
}

static void sendAdminConfig() {
  if (!isAdmin()) {
    requireAdminOr401();
    return;
  }

  JsonDocument d;
  JsonObject c = d["config"].to<JsonObject>();
  c["hostname"] = cfg.hostname;
  c["wifi_static_enabled"] = cfg.wifiStaticEnabled;
  c["wifi_static_ip"] = cfg.wifiStaticIp;
  c["wifi_gateway"] = cfg.wifiGateway;
  c["wifi_subnet"] = cfg.wifiSubnet;
  c["wifi_dns1"] = cfg.wifiDns1;
  c["wifi_dns2"] = cfg.wifiDns2;
  c["shelly_ip"] = cfg.shellyIp;
  c["shelly_profile"] = cfg.shellyMonophase ? "mono" : "tri";
  c["shelly_grid_channel"] = cfg.shellyGridChannel;
  c["shelly_routed_channel"] = cfg.shellyRoutedChannel;
  c["shelly_aux_channel"] = cfg.shellyAuxChannel;
  c["shelly_aux_label"] = cfg.shellyAuxLabel;
  c["f1atb_ip"] = cfg.f1atbIp;
  c["f1atb_enabled"] = cfg.f1atbEnabled;
  c["f1atb_action_label"] = cfg.f1atbActionLabel;
  c["f1atb_action_number"] = cfg.f1atbActionNumber;
  c["weather_enabled"] = cfg.weatherEnabled;
  c["weather_city"] = cfg.weatherCity;
  c["weather_display_name"] = cfg.weatherDisplayName;
  c["weather_lat"] = cfg.weatherLat;
  c["weather_lon"] = cfg.weatherLon;
  c["charge_w"] = cfg.totalChargeW;
  c["zendure_writes"] = cfg.zendureWritesEnabled;

  JsonArray z = d["zendure"].to<JsonArray>();
  for (size_t i = 0; i < zendureCount; ++i) {
    JsonObject o = z.add<JsonObject>();
    o["configured"] = zendures[i].configured;
    o["enabled"] = zendures[i].enabled;
    o["label"] = zendures[i].label;
    o["sn"] = zendures[i].sn;
    o["manual_ip"] = zendures[i].manualIp;
    o["ip"] = zendures[i].ip;
    o["online"] = zendures[i].online;
  }

  String out;
  serializeJson(d, out);
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "application/json", out);
}

static void sendWifiScan() {
  const int n = WiFi.scanNetworks(false, true);

  JsonDocument d;
  JsonArray networks = d["networks"].to<JsonArray>();

  for (int i = 0; i < n && i < 25; ++i) {
    JsonObject o = networks.add<JsonObject>();

    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
    o["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
  }

  WiFi.scanDelete();

  String out;
  serializeJson(d, out);
  server.send(200, "application/json", out);
}

void setupWebServer() {
  const char* headers[] = {"Cookie"};
  server.collectHeaders(headers, 1);

  server.on("/", HTTP_GET, []() {
    server.send(
      200,
      "text/html; charset=utf-8",
      isSetupPortal() ? setupHtml() : dashboardHtml()
    );
  });

  server.on("/admin", HTTP_GET, []() {
    server.send(
      200,
      "text/html; charset=utf-8",
      adminHtml(isAdmin())
    );
  });

  server.on("/api/auth/status", HTTP_GET, []() {
    JsonDocument d;
    d["admin"] = isAdmin();

    String body;
    serializeJson(d, body);
    server.send(200, "application/json", body);
  });

  server.on("/api/status", HTTP_GET, sendStatus);
  server.on("/api/config", HTTP_GET, sendAdminConfig);
  server.on("/api/history", HTTP_GET, sendHistory);

  server.on("/api/login", HTTP_POST, []() {
    JsonDocument d;

    if (!parseBody(d)) {
      server.send(400, "application/json", "{}");
      return;
    }

    const String password = d["password"] | "";

    if (!cfg.adminPass.isEmpty() && password == cfg.adminPass) {
      sessionToken = randomToken();

      server.sendHeader(
        "Set-Cookie",
        "zt_session=" + sessionToken + "; Path=/; HttpOnly; SameSite=Strict"
      );

      server.send(200, "application/json", R"({"ok":true})");
    } else {
      server.send(403, "application/json", R"({"ok":false})");
    }
  });

  server.on("/api/logout", HTTP_POST, []() {
    if (!isAdmin()) {
      requireAdminOr401();
      return;
    }

    sessionToken = "";

    server.sendHeader(
      "Set-Cookie",
      "zt_session=; Max-Age=0; Path=/; SameSite=Strict"
    );

    server.send(200, "application/json", R"({"ok":true})");
  });

  server.on("/api/control", HTTP_POST, []() {
    if (!isAdmin()) {
      requireAdminOr401();
      return;
    }

    JsonDocument d;

    if (!parseBody(d)) {
      server.send(400, "application/json", R"({"ok":false})");
      return;
    }

    const String mode = d["mode"] | "auto";

    const int minutes = constrain(
      static_cast<int>(d["minutes"] | 60),
      MIN_MANUAL_TIMEOUT_MIN,
      MAX_MANUAL_TIMEOUT_MIN
    );

    const int powerW = constrain(
      static_cast<int>(d["power_w"] | cfg.totalChargeW),
      0,
      MAX_TOTAL_CHARGE_W
    );

    cfg.totalChargeW = powerW;
    saveConfig();

    if (mode == "auto") {
      requestedMode = ControlMode::AUTO;
      manualUntilEpoch = 0;
      manualUntilMs = 0;
    } else {
      requestedMode =
        mode == "charge"
          ? ControlMode::FORCE_CHARGE
          : ControlMode::SELF_CONSUMPTION;

      const uint32_t timeoutMs =
        static_cast<uint32_t>(minutes) * 60UL * 1000UL;

      manualUntilMs = millis() + timeoutMs;

      const uint32_t now = nowEpoch();
      manualUntilEpoch =
        now != 0
          ? now + static_cast<uint32_t>(minutes) * 60UL
          : 0;
    }

    evaluateControl(true);
    server.send(200, "application/json", R"({"ok":true})");
  });

  server.on("/api/config", HTTP_POST, []() {
    if (!isAdmin()) {
      requireAdminOr401();
      return;
    }

    JsonDocument d;

    if (!parseBody(d)) {
      server.send(400, "application/json", "{}");
      return;
    }

    const bool rebootAfterSave = d["reboot"] | false;

    struct FleetSnapshot {
      bool configured;
      bool enabled;
      String sn;
      String manualIp;
    };

    FleetSnapshot oldFleet[MAX_ZENDURE_DEVICES];
    const size_t oldZendureCount = zendureCount;

    for (size_t i = 0; i < MAX_ZENDURE_DEVICES; ++i) {
      oldFleet[i].configured = zendures[i].configured;
      oldFleet[i].enabled = zendures[i].enabled;
      oldFleet[i].sn = zendures[i].sn;
      oldFleet[i].manualIp = zendures[i].manualIp;
    }

    String hostname = d["hostname"] | cfg.hostname;
    String shelly = d["shelly_ip"] | cfg.shellyIp;
    String f1atb = d["f1atb_ip"] | cfg.f1atbIp;
    const bool wifiStaticEnabled =
      d["wifi_static_enabled"] | cfg.wifiStaticEnabled;
    String wifiStaticIp = d["wifi_static_ip"] | cfg.wifiStaticIp;
    String wifiGateway = d["wifi_gateway"] | cfg.wifiGateway;
    String wifiSubnet = d["wifi_subnet"] | cfg.wifiSubnet;
    String wifiDns1 = d["wifi_dns1"] | cfg.wifiDns1;
    String wifiDns2 = d["wifi_dns2"] | cfg.wifiDns2;

    const String shellyProfile = d["shelly_profile"] | (cfg.shellyMonophase ? "mono" : "tri");
    const uint8_t shellyGridChannel = constrain(static_cast<int>(d["shelly_grid_channel"] | cfg.shellyGridChannel), 0, 2);
    const int shellyRoutedRaw = static_cast<int>(d["shelly_routed_channel"] | cfg.shellyRoutedChannel);
    const int8_t shellyRoutedChannel =
      (shellyRoutedRaw >= 0 && shellyRoutedRaw <= 2) ? shellyRoutedRaw : -1;
    const int shellyAuxRaw = static_cast<int>(d["shelly_aux_channel"] | cfg.shellyAuxChannel);
    const int8_t shellyAuxChannel =
      (shellyAuxRaw >= 0 && shellyAuxRaw <= 2) ? shellyAuxRaw : -1;

    String shellyAuxLabel = d["shelly_aux_label"] | cfg.shellyAuxLabel;
    shellyAuxLabel.trim();
    if (shellyAuxLabel.isEmpty()) shellyAuxLabel = "Canal personnalisé";

    hostname.trim();
    shelly.trim();
    f1atb.trim();
    wifiStaticIp.trim();
    wifiGateway.trim();
    wifiSubnet.trim();
    wifiDns1.trim();
    wifiDns2.trim();

    if (wifiStaticEnabled) {
      IPAddress ip, gw, sub, dns1, dns2;
      if (
        !ip.fromString(wifiStaticIp) ||
        !gw.fromString(wifiGateway) ||
        !sub.fromString(wifiSubnet) ||
        (!wifiDns1.isEmpty() && !dns1.fromString(wifiDns1)) ||
        (!wifiDns2.isEmpty() && !dns2.fromString(wifiDns2))
      ) {
        server.send(
          400,
          "application/json",
          R"({"ok":false,"error":"invalid_static_ip","message":"Configuration IP fixe invalide."})"
        );
        return;
      }
    }

    if (shellyProfile != "tri") {
      if (
        (shellyRoutedChannel >= 0 && shellyRoutedChannel == shellyGridChannel) ||
        (shellyAuxChannel >= 0 && shellyAuxChannel == shellyGridChannel) ||
        (
          shellyRoutedChannel >= 0 &&
          shellyAuxChannel >= 0 &&
          shellyRoutedChannel == shellyAuxChannel
        )
      ) {
        server.send(
          409,
          "application/json",
          R"({"ok":false,"error":"duplicate_shelly_channel","message":"Chaque fonction Shelly doit utiliser un canal différent."})"
        );
        return;
      }
    }

    const int powerW = constrain(
      static_cast<int>(d["charge_w"] | cfg.totalChargeW),
      0,
      MAX_TOTAL_CHARGE_W
    );

    const bool zendureWrites =
      d["zendure_writes"] | cfg.zendureWritesEnabled;

    const bool zendureWritesWasEnabled = cfg.zendureWritesEnabled;

    const bool f1atbEnabled =
      d["f1atb_enabled"] | cfg.f1atbEnabled;

    String f1atbActionLabel =
      d["f1atb_action_label"] | cfg.f1atbActionLabel;

    const int f1atbActionNumber =
      constrain(static_cast<int>(d["f1atb_action_number"] | cfg.f1atbActionNumber), 0, 15);

    f1atbActionLabel.trim();
    if (f1atbActionLabel.isEmpty()) f1atbActionLabel = "Chauffe-eau";
    const bool weatherEnabled =
      d["weather_enabled"] | cfg.weatherEnabled;

    const String weatherCity =
      d["weather_city"] | cfg.weatherCity;

    const String weatherDisplayName =
      d["weather_display_name"] | cfg.weatherDisplayName;

    const double weatherLat =
      d["weather_lat"] | cfg.weatherLat;

    const double weatherLon =
      d["weather_lon"] | cfg.weatherLon;

    if (hostname.length() < 1 || hostname.length() > 31) {
      server.send(400, "application/json", R"({"error":"hostname"})");
      return;
    }

    if (d["zendure"].is<JsonArray>()) {
      JsonArray nextArr = d["zendure"].as<JsonArray>();

      for (size_t i = 0; i < nextArr.size(); ++i) {
        JsonObject a = nextArr[i];
        const bool aEnabled = a["enabled"] | true;
        if (!aEnabled) continue;

        String aSn = String((const char*)(a["sn"] | ""));
        aSn.trim();
        if (aSn.isEmpty()) continue;

        for (size_t j = i + 1; j < nextArr.size(); ++j) {
          JsonObject b = nextArr[j];
          const bool bEnabled = b["enabled"] | true;
          if (!bEnabled) continue;

          String bSn = String((const char*)(b["sn"] | ""));
          bSn.trim();
          if (bSn.isEmpty()) continue;

          if (aSn.equalsIgnoreCase(bSn)) {
            server.send(
              409,
              "application/json",
              R"({"ok":false,"error":"duplicate_zendure_sn","message":"Configuration refusée : deux Zendure actifs utilisent le même numéro de série."})"
            );
            return;
          }
        }
      }
    }

    // Release devices that are about to leave this controller BEFORE mutating
    // their identity/configuration. Matching is by SN, so reordering rows does
    // not cause an unnecessary release.
    if (cfg.zendureWritesEnabled && d["zendure"].is<JsonArray>()) {
      JsonArray nextArr = d["zendure"].as<JsonArray>();

      for (size_t i = 0; i < zendureCount; ++i) {
        ZendureDevice& old = zendures[i];

        if (!old.configured || !old.enabled) continue;

        bool stillManaged = false;

        for (JsonObject next : nextArr) {
          const bool nextEnabled = next["enabled"] | true;
          if (!nextEnabled) continue;

          String nextSn = String((const char*)(next["sn"] | ""));
          nextSn.trim();

          if (
            !old.sn.isEmpty() &&
            !nextSn.isEmpty() &&
            old.sn.equalsIgnoreCase(nextSn)
          ) {
            stillManaged = true;
            break;
          }
        }

        if (!stillManaged) {
          if (!releaseZendureIndexNow(i)) {
            server.send(
              409,
              "application/json",
              R"({"ok":false,"error":"zendure_remove_release_failed","message":"Configuration refusée : un Zendure supprimé, désactivé ou remplacé n'a pas pu être libéré vers HEMS."})"
            );
            return;
          }
        }
      }
    }

    // Disabling all physical writes is stricter: every currently managed
    // device must be reachable and released first, otherwise keep writes ON.
    if (cfg.zendureWritesEnabled && !zendureWrites) {
      for (size_t i = 0; i < zendureCount; ++i) {
        if (!zendures[i].configured || !zendures[i].enabled) continue;

        if (!releaseZendureIndexNow(i)) {
          server.send(
            409,
            "application/json",
            R"({"ok":false,"error":"zendure_release_failed","message":"Impossible de désactiver les écritures : au moins un Zendure n'a pas pu être libéré vers HEMS."})"
          );
          return;
        }
      }

      requestedMode = ControlMode::AUTO;
      appliedMode = ControlMode::SELF_CONSUMPTION;
      manualUntilEpoch = 0;
      manualUntilMs = 0;
    }

    // Preserve runtime release obligations by stable SN before rebuilding the
    // configurable Zendure array. This makes row reordering harmless.
    String pendingReleaseSn[MAX_ZENDURE_DEVICES];
    size_t pendingReleaseCount = 0;

    for (size_t i = 0; i < zendureCount; ++i) {
      if (
        zendures[i].configured &&
        zendures[i].releasePending &&
        !zendures[i].sn.isEmpty() &&
        pendingReleaseCount < MAX_ZENDURE_DEVICES
      ) {
        pendingReleaseSn[pendingReleaseCount++] = zendures[i].sn;
      }
    }

    const bool oldShellyMonophase = cfg.shellyMonophase;
    const uint8_t oldShellyGridChannel = cfg.shellyGridChannel;
    const int8_t oldShellyRoutedChannel = cfg.shellyRoutedChannel;
    const int8_t oldShellyAuxChannel = cfg.shellyAuxChannel;

    cfg.hostname = hostname;
    cfg.shellyIp = shelly;
    cfg.wifiStaticEnabled = wifiStaticEnabled;
    cfg.wifiStaticIp = wifiStaticIp;
    cfg.wifiGateway = wifiGateway;
    cfg.wifiSubnet = wifiSubnet;
    cfg.wifiDns1 = wifiDns1;
    cfg.wifiDns2 = wifiDns2;
    cfg.shellyMonophase = shellyProfile != "tri";
    cfg.shellyGridChannel = shellyGridChannel;
    cfg.shellyRoutedChannel = cfg.shellyMonophase ? shellyRoutedChannel : -1;
    cfg.shellyAuxChannel = cfg.shellyMonophase ? shellyAuxChannel : -1;
    cfg.shellyAuxLabel = shellyAuxLabel;

    if (oldShellyMonophase != cfg.shellyMonophase || oldShellyGridChannel != cfg.shellyGridChannel) {
      shellyGridImportDayBaseWh=NAN; shellyGridExportDayBaseWh=NAN; shellyGridImportTodayWh=NAN; shellyGridExportTodayWh=NAN;
    }
    if (oldShellyRoutedChannel != cfg.shellyRoutedChannel) { shellyRoutedDayBaseWh=NAN; shellyRoutedTodayWh=NAN; }
    if (oldShellyAuxChannel != cfg.shellyAuxChannel) { shellyAuxDayBaseWh=NAN; shellyAuxTodayWh=NAN; }
    cfg.f1atbIp = f1atb;
    cfg.f1atbEnabled = f1atbEnabled;
    cfg.f1atbActionLabel = f1atbActionLabel;
    cfg.f1atbActionNumber = static_cast<uint8_t>(f1atbActionNumber);
    cfg.weatherEnabled = weatherEnabled;
    cfg.weatherCity = weatherCity;
    cfg.weatherDisplayName = weatherDisplayName;
    cfg.weatherLat = weatherLat;
    cfg.weatherLon = weatherLon;
    cfg.totalChargeW = powerW;
    cfg.zendureWritesEnabled = zendureWrites;

    if (d["zendure"].is<JsonArray>()) {
      JsonArray arr = d["zendure"].as<JsonArray>();
      zendureCount = min<size_t>(arr.size(), MAX_ZENDURE_DEVICES);

      for (size_t i = 0; i < MAX_ZENDURE_DEVICES; ++i) {
        if (i < zendureCount) {
          JsonObject z = arr[i];
          zendures[i].configured = true;
          zendures[i].enabled = z["enabled"] | true;
          zendures[i].label = String((const char*)(z["label"] | ""));
          zendures[i].sn = String((const char*)(z["sn"] | ""));
          zendures[i].manualIp = String((const char*)(z["manual_ip"] | ""));
          zendures[i].label.trim();
          zendures[i].sn.trim();
          zendures[i].manualIp.trim();

          zendures[i].releasePending = false;
          for (size_t p = 0; p < pendingReleaseCount; ++p) {
            if (
              !pendingReleaseSn[p].isEmpty() &&
              zendures[i].sn.equalsIgnoreCase(pendingReleaseSn[p])
            ) {
              zendures[i].releasePending = true;
              break;
            }
          }

          if (zendures[i].label.isEmpty()) {
            zendures[i].label = "SolarFlow " + String(i + 1);
          }

          // Runtime address is cleared when no manual override exists;
          // mDNS will refill it using the configured SN.
          if (!zendures[i].manualIp.isEmpty()) {
            zendures[i].ip = zendures[i].manualIp;
            zendures[i].port = 80;
          } else {
            zendures[i].ip = "";
            zendures[i].port = 80;
          }
          zendures[i].online = false;
        } else {
          zendures[i] = ZendureDevice{};
        }
      }
    }


    // Compare the managed fleet by identity instead of by row index.
    // Reordering the UI alone must not trigger a physical HEMS release.
    auto containsActiveDevice = [](const String& sn, const String& manualIp) {
      for (size_t j = 0; j < zendureCount; ++j) {
        const ZendureDevice& z = zendures[j];
        if (!z.configured || !z.enabled) continue;

        if (
          !sn.isEmpty() &&
          !z.sn.isEmpty() &&
          sn.equalsIgnoreCase(z.sn)
        ) {
          return true;
        }

        if (
          sn.isEmpty() &&
          !manualIp.isEmpty() &&
          z.sn.isEmpty() &&
          manualIp == z.manualIp
        ) {
          return true;
        }
      }
      return false;
    };

    size_t oldActiveCount = 0;
    size_t newActiveCount = 0;
    bool fleetChanged = false;

    for (size_t i = 0; i < MAX_ZENDURE_DEVICES; ++i) {
      if (oldFleet[i].configured && oldFleet[i].enabled) {
        ++oldActiveCount;
        if (!containsActiveDevice(oldFleet[i].sn, oldFleet[i].manualIp)) {
          fleetChanged = true;
        }
      }

      if (zendures[i].configured && zendures[i].enabled) {
        ++newActiveCount;
      }
    }

    if (oldActiveCount != newActiveCount) {
      fleetChanged = true;
    }

    const bool writesJustEnabled =
      !zendureWritesWasEnabled && cfg.zendureWritesEnabled;

    if (
      cfg.zendureWritesEnabled &&
      (fleetChanged || writesJustEnabled) &&
      effectiveMode() == ControlMode::SELF_CONSUMPTION
    ) {
      markZendureReleasePending();
    }

    saveConfig();

    server.send(
      200,
      "application/json",
      rebootAfterSave
        ? R"({"ok":true,"saved":true,"rebooting":true})"
        : R"({"ok":true,"saved":true,"note":"Parametres enregistres. Les parametres reseau seront appliques au prochain redemarrage."})"
    );

    if (rebootAfterSave) {
      delay(500);
      ESP.restart();
    }
  });

  server.on("/api/f1atb/on", HTTP_POST, []() {
    if (!cfg.f1atbEnabled) {
      server.send(403, "application/json", R"({"ok":false,"error":"f1atb_disabled"})");
      return;
    }

    const bool ok = stepF1ATBOn();

    server.send(
      ok ? 200 : 502,
      "application/json",
      ok ? R"({"ok":true})" : R"({"ok":false,"error":"f1atb_unreachable"})"
    );
  });

  server.on("/api/f1atb/off", HTTP_POST, []() {
    if (!cfg.f1atbEnabled) {
      server.send(403, "application/json", R"({"ok":false,"error":"f1atb_disabled"})");
      return;
    }

    const bool ok = stepF1ATBOff();

    server.send(
      ok ? 200 : 502,
      "application/json",
      ok ? R"({"ok":true})" : R"({"ok":false,"error":"f1atb_unreachable"})"
    );
  });

  server.on("/api/f1atb/cancel", HTTP_POST, []() {
    if (!cfg.f1atbEnabled) {
      server.send(403, "application/json", R"({"ok":false,"error":"f1atb_disabled"})");
      return;
    }

    const bool ok = cancelF1ATBForce();

    server.send(
      ok ? 200 : 502,
      "application/json",
      ok ? R"({"ok":true})" : R"({"ok":false,"error":"f1atb_unreachable"})"
    );
  });

  server.on(
    "/api/system/ota",
    HTTP_POST,
    []() {
      if (!isAdmin()) {
        requireAdminOr401();
        return;
      }

      const bool ok = !Update.hasError();

      server.send(
        ok ? 200 : 500,
        "application/json",
        ok
          ? R"({"ok":true,"rebooting":true})"
          : R"({"ok":false,"error":"update_failed"})"
      );

      if (ok) {
        delay(400);
        ESP.restart();
      }
    },
    []() {
      if (!isAdmin()) {
        return;
      }

      HTTPUpload& upload = server.upload();

      if (upload.status == UPLOAD_FILE_START) {
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (!Update.end(true)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.abort();
      }
    }
  );

  server.on("/api/system/reboot", HTTP_POST, []() {
    if (!isAdmin()) {
      requireAdminOr401();
      return;
    }

    server.send(200, "application/json", R"({"ok":true,"rebooting":true})");
    delay(250);
    ESP.restart();
  });

  server.on("/api/system/factory-reset", HTTP_POST, []() {
    if (!isAdmin()) {
      requireAdminOr401();
      return;
    }

    Preferences prefs;
    prefs.begin("ztempo", false);
    prefs.clear();
    prefs.end();

    server.send(200, "application/json", R"({"ok":true,"factory_reset":true,"rebooting":true})");
    delay(300);
    ESP.restart();
  });

  server.on("/api/weather/search", HTTP_GET, []() {
    if (!isAdmin()) {
      requireAdminOr401();
      return;
    }

    if (!server.hasArg("q")) {
      server.send(400, "application/json", R"({"error":"missing_query"})");
      return;
    }

    JsonDocument upstream;

    if (!searchWeatherCity(server.arg("q"), upstream)) {
      server.send(502, "application/json", R"({"error":"geocoding_failed"})");
      return;
    }

    JsonDocument response;
    JsonArray out = response["results"].to<JsonArray>();
    JsonArray results = upstream["results"].as<JsonArray>();

    for (JsonObject item : results) {
      JsonObject o = out.add<JsonObject>();
      o["name"] = item["name"] | "";
      o["admin1"] = item["admin1"] | "";
      o["country"] = item["country"] | "";
      o["country_code"] = item["country_code"] | "";
      o["latitude"] = item["latitude"] | 0.0;
      o["longitude"] = item["longitude"] | 0.0;
    }

    String body;
    serializeJson(response, body);
    server.send(200, "application/json", body);
  });

  server.on("/api/wifi/scan", HTTP_GET, []() {
    if (!isAdmin()) {
      requireAdminOr401();
      return;
    }

    sendWifiScan();
  });

  server.on("/api/wifi/save", HTTP_POST, []() {
    if (!isAdmin()) {
      requireAdminOr401();
      return;
    }

    JsonDocument d;

    if (!parseBody(d)) {
      server.send(400, "application/json", "{}");
      return;
    }

    const String ssid = d["ssid"] | "";
    const String password = d["password"] | "";

    if (ssid.isEmpty()) {
      server.send(400, "application/json", R"({"error":"ssid"})");
      return;
    }

    cfg.wifiSsid = ssid;
    cfg.wifiPass = password;
    saveConfig();

    server.send(200, "application/json", R"({"ok":true,"reboot":true})");
    delay(300);
    ESP.restart();
  });

  server.on("/api/zendure/discover", HTTP_POST, []() {
    if (!isAdmin()) {
      requireAdminOr401();
      return;
    }

    discoverZendure();
    pollZendures();

    server.send(200, "application/json", R"({"ok":true})");
  });

  server.on("/api/setup/scan", HTTP_GET, []() {
    if (!isSetupPortal()) {
      server.send(404);
      return;
    }

    sendWifiScan();
  });

  server.on("/api/setup/save", HTTP_POST, []() {
    if (!isSetupPortal()) {
      server.send(404);
      return;
    }

    JsonDocument d;

    if (!parseBody(d)) {
      server.send(400);
      return;
    }

    const String ssid = d["ssid"] | "";
    const String wifiPass = d["password"] | "";
    const String adminPass = d["admin"] | "";

    if (ssid.isEmpty() || adminPass.length() < 8) {
      server.send(
        400,
        "text/plain",
        "SSID requis et mot de passe admin >= 8 caractères."
      );
      return;
    }

    cfg.wifiSsid = ssid;
    cfg.wifiPass = wifiPass;
    cfg.adminPass = adminPass;
    saveConfig();

    server.send(
      200,
      "text/plain",
      "Configuration enregistrée. Redémarrage..."
    );

    delay(500);
    ESP.restart();
  });

  server.onNotFound([]() {
    if (isSetupPortal()) {
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "Not found");
    }
  });

  server.begin();
}

void handleWebServer() {
  server.handleClient();
}
