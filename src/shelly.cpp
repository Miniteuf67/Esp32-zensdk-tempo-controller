#include <ArduinoJson.h>
#include <math.h>
#include "shelly.h"
#include "app_state.h"
#include "http_helpers.h"
#include "settings.h"

static String localDay() {
  struct tm t;
  if (!localTimeSafe(t)) return "";
  char buf[11];
  strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
  return String(buf);
}

static float positiveDelta(float total, float base) {
  if (isnan(total) || isnan(base)) return NAN;
  const float d = total - base;
  return d >= 0.0f ? d : 0.0f;
}

static void updateDailyEnergy() {
  const String day = localDay();
  if (day.isEmpty()) return;
  bool save = false;
  if (shellyEnergyDayDate != day) {
    shellyEnergyDayDate = day;
    shellyGridImportDayBaseWh = shellyGridImportTotalWh;
    shellyGridExportDayBaseWh = shellyGridExportTotalWh;
    shellyRoutedDayBaseWh = shellyRoutedTotalWh;
    shellyAuxDayBaseWh = shellyAuxTotalWh;
    save = true;
  } else {
    if (isnan(shellyGridImportDayBaseWh) && !isnan(shellyGridImportTotalWh)) { shellyGridImportDayBaseWh = shellyGridImportTotalWh; save = true; }
    if (isnan(shellyGridExportDayBaseWh) && !isnan(shellyGridExportTotalWh)) { shellyGridExportDayBaseWh = shellyGridExportTotalWh; save = true; }
    if (isnan(shellyRoutedDayBaseWh) && !isnan(shellyRoutedTotalWh)) { shellyRoutedDayBaseWh = shellyRoutedTotalWh; save = true; }
    if (isnan(shellyAuxDayBaseWh) && !isnan(shellyAuxTotalWh)) { shellyAuxDayBaseWh = shellyAuxTotalWh; save = true; }
  }
  shellyGridImportTodayWh = positiveDelta(shellyGridImportTotalWh, shellyGridImportDayBaseWh);
  shellyGridExportTodayWh = positiveDelta(shellyGridExportTotalWh, shellyGridExportDayBaseWh);
  shellyRoutedTodayWh = positiveDelta(shellyRoutedTotalWh, shellyRoutedDayBaseWh);
  shellyAuxTodayWh = positiveDelta(shellyAuxTotalWh, shellyAuxDayBaseWh);
  if (save) saveShellyEnergyBaselines();
}

static bool pollGridMonophase() {
  JsonDocument doc;
  const String url = "http://" + cfg.shellyIp + "/rpc/EM1.GetStatus?id=" + String(cfg.shellyGridChannel);
  if (!httpGetJson(url, doc, 2500)) return false;
  shellyPowerW = doc["act_power"] | NAN;
  shellyVoltageV = doc["voltage"] | NAN;
  shellyCurrentA = doc["current"] | NAN;
  shellyPf = doc["pf"] | NAN;
  shellyLastOkMs = millis();
  return true;
}

static bool pollGridTriphase() {
  JsonDocument doc;
  const String url = "http://" + cfg.shellyIp + "/rpc/EM.GetStatus?id=0";
  if (!httpGetJson(url, doc, 2500)) return false;
  shellyPowerW = doc["total_act_power"] | NAN;
  shellyVoltageV = doc["a_voltage"] | NAN;
  shellyCurrentA = doc["a_current"] | NAN;
  shellyPf = doc["a_pf"] | NAN;
  shellyLastOkMs = millis();
  return true;
}

static bool pollEm1Energy(int8_t channel, float& actWh, float& retWh) {
  if (channel < 0 || channel > 2) return false;
  JsonDocument data;
  const String url = "http://" + cfg.shellyIp + "/rpc/EM1Data.GetStatus?id=" + String(channel);
  if (!httpGetJson(url, data, 2500)) return false;
  actWh = data["total_act_energy"] | NAN;
  retWh = data["total_act_ret_energy"] | NAN;
  return true;
}

static void pollGridEnergy() {
  JsonDocument data;
  const String url = cfg.shellyMonophase
    ? "http://" + cfg.shellyIp + "/rpc/EM1Data.GetStatus?id=" + String(cfg.shellyGridChannel)
    : "http://" + cfg.shellyIp + "/rpc/EMData.GetStatus?id=0";

  if (!httpGetJson(url, data, 2500)) return;

  if (cfg.shellyMonophase) {
    // EM1Data fields
    shellyGridImportTotalWh = data["total_act_energy"] | NAN;
    shellyGridExportTotalWh = data["total_act_ret_energy"] | NAN;
  } else {
    // EMData fields
    shellyGridImportTotalWh = data["total_act"] | NAN;
    shellyGridExportTotalWh = data["total_act_ret"] | NAN;
  }
}

static void pollRoutedMonophase(bool readEnergy) {
  if (cfg.shellyRoutedChannel < 0 || cfg.shellyRoutedChannel > 2) {
    shellyRoutedPowerW=NAN; shellyRoutedEnergyWh=NAN; shellyRoutedTotalWh=NAN; shellyRoutedTodayWh=NAN; shellyRoutedLastOkMs=0; return;
  }
  JsonDocument status;
  const String url="http://"+cfg.shellyIp+"/rpc/EM1.GetStatus?id="+String(cfg.shellyRoutedChannel);
  if (httpGetJson(url,status,2500)) { shellyRoutedPowerW=status["act_power"]|NAN; shellyRoutedLastOkMs=millis(); }
  if (!readEnergy) return;
  float act=NAN, ret=NAN;
  if (pollEm1Energy(cfg.shellyRoutedChannel,act,ret)) {
    shellyRoutedTotalWh=(isnan(act)?0.0f:act)+(isnan(ret)?0.0f:ret);
    shellyRoutedEnergyWh=shellyRoutedTotalWh;
  }
}

static void pollAuxMonophase(bool readEnergy) {
  if (cfg.shellyAuxChannel < 0 || cfg.shellyAuxChannel > 2) {
    shellyAuxPowerW=NAN; shellyAuxTotalWh=NAN; shellyAuxTodayWh=NAN; shellyAuxLastOkMs=0; return;
  }
  JsonDocument status;
  const String url="http://"+cfg.shellyIp+"/rpc/EM1.GetStatus?id="+String(cfg.shellyAuxChannel);
  if (httpGetJson(url,status,2500)) { shellyAuxPowerW=status["act_power"]|NAN; shellyAuxLastOkMs=millis(); }
  if (!readEnergy) return;
  float act=NAN, ret=NAN;
  if (pollEm1Energy(cfg.shellyAuxChannel,act,ret)) shellyAuxTotalWh=(isnan(act)?0.0f:act)+(isnan(ret)?0.0f:ret);
}

void pollShelly() {
  if (cfg.shellyIp.isEmpty()) return;
  static uint8_t energyDivider=0;
  const bool readEnergy=(++energyDivider>=10);
  if (readEnergy) energyDivider=0;
  if (cfg.shellyMonophase) {
    pollGridMonophase();
    pollRoutedMonophase(readEnergy);
    pollAuxMonophase(readEnergy);
  } else {
    pollGridTriphase();
    shellyRoutedPowerW=NAN; shellyRoutedEnergyWh=NAN; shellyRoutedTotalWh=NAN; shellyRoutedTodayWh=NAN; shellyRoutedLastOkMs=0;
    shellyAuxPowerW=NAN; shellyAuxTotalWh=NAN; shellyAuxTodayWh=NAN; shellyAuxLastOkMs=0;
  }
  if (readEnergy) { pollGridEnergy(); updateDailyEnergy(); }
  static uint8_t divider=0;
  if (++divider>=15) {
    divider=0;
    JsonDocument global;
    if (httpGetJson("http://"+cfg.shellyIp+"/rpc/Shelly.GetStatus",global,2500)) shellyRssi=global["wifi"]["rssi"]|-127;
  }
}
