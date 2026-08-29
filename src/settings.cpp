#include <Preferences.h>
#include "settings.h"
#include "app_state.h"
#include "config.h"

static Preferences prefs;

void loadConfig() {
  prefs.begin("ztempo", true);

  cfg.hostname = prefs.getString("hostname", DEFAULT_HOSTNAME);
  cfg.shellyIp = prefs.getString("shelly", DEFAULT_SHELLY_IP);
  cfg.shellyMonophase = prefs.getBool("shmono", true);
  cfg.shellyGridChannel = min<uint8_t>(prefs.getUChar("shgrid", 0), 2);
  {
    const int routed = prefs.getChar("shrout", 1);
    cfg.shellyRoutedChannel = (routed >= 0 && routed <= 2) ? routed : -1;
  }
  {
    const int aux = prefs.getChar("shaux", prefs.getChar("shpv", -1));
    cfg.shellyAuxChannel = (aux >= 0 && aux <= 2) ? aux : -1;
  }
  cfg.shellyAuxLabel = prefs.getString("shauxlbl", "Canal personnalisé");
  shellyEnergyDayDate = prefs.getString("shday", "");
  shellyGridImportDayBaseWh = prefs.getFloat("shgib", NAN);
  shellyGridExportDayBaseWh = prefs.getFloat("shgeb", NAN);
  shellyRoutedDayBaseWh = prefs.getFloat("shrb", NAN);
  shellyAuxDayBaseWh = prefs.getFloat("shauxb", prefs.getFloat("shpvb", NAN));
  pvEnergyTotalWh = prefs.getDouble("pvtot", 0.0);
  pvEnergyDayBaseWh = prefs.getDouble("pvbase", pvEnergyTotalWh);
  pvEnergyDayDate = prefs.getString("pvday", "");
  pvEnergyTodayWh = max(0.0, pvEnergyTotalWh - pvEnergyDayBaseWh);
  cfg.f1atbIp = prefs.getString("f1atb", DEFAULT_F1ATB_IP);
  cfg.f1atbEnabled = prefs.getBool("f1en", false);
  cfg.f1atbActionLabel = prefs.getString("f1lbl", "Chauffe-eau");
  cfg.f1atbActionNumber = prefs.getUChar("f1act", 0);

  cfg.weatherEnabled = prefs.getBool("wen", false);
  cfg.weatherCity = prefs.getString("wcity", "");
  cfg.weatherDisplayName = prefs.getString("wdisp", "");
  cfg.weatherLat = prefs.getDouble("wlat", 0.0);
  cfg.weatherLon = prefs.getDouble("wlon", 0.0);
  cfg.wifiSsid = prefs.getString("ssid", "");
  cfg.wifiPass = prefs.getString("wpass", "");
  cfg.wifiStaticEnabled = prefs.getBool("wstatic", false);
  cfg.wifiStaticIp = prefs.getString("wip", "");
  cfg.wifiGateway = prefs.getString("wgw", "");
  cfg.wifiSubnet = prefs.getString("wsub", "255.255.255.0");
  cfg.wifiDns1 = prefs.getString("wdns1", "");
  cfg.wifiDns2 = prefs.getString("wdns2", "");
  cfg.adminPass = prefs.getString("apass", "");
  cfg.totalChargeW = prefs.getUShort("chargeW", DEFAULT_TOTAL_CHARGE_W);
  cfg.zendureWritesEnabled = prefs.getBool("zwrite", false);

  tempoToday = static_cast<TempoColor>(prefs.getUChar("tToday", 0));
  tempoTomorrow = static_cast<TempoColor>(prefs.getUChar("tTomorrow", 0));
  tempoTodayDate = prefs.getString("tTodayD", "");
  tempoTomorrowDate = prefs.getString("tTomD", "");

  zendureCount = min<size_t>(prefs.getUChar("zcount", 0), MAX_ZENDURE_DEVICES);
  for (size_t i = 0; i < MAX_ZENDURE_DEVICES; ++i) {
    const String k = "z" + String(i);
    zendures[i].configured = i < zendureCount;
    zendures[i].enabled = prefs.getBool((k + "en").c_str(), true);
    zendures[i].label = prefs.getString((k + "lbl").c_str(), "SolarFlow " + String(i + 1));
    zendures[i].sn = prefs.getString((k + "sn").c_str(), "");
    zendures[i].manualIp = prefs.getString((k + "ip").c_str(), "");
  }

  prefs.end();

  Serial.println("[CONFIG] Configuration relue depuis NVS");
  Serial.printf("[CONFIG] Hostname=%s WiFi=%s IP=%s\n",
                cfg.hostname.c_str(),
                cfg.wifiStaticEnabled ? "statique" : "DHCP",
                cfg.wifiStaticEnabled ? cfg.wifiStaticIp.c_str() : "auto");
  Serial.printf("[CONFIG] GW=%s DNS1=%s DNS2=%s\n",
                cfg.wifiGateway.c_str(), cfg.wifiDns1.c_str(), cfg.wifiDns2.c_str());
  Serial.printf("[CONFIG] Meteo=%s ville=%s lat=%.6f lon=%.6f\n",
                cfg.weatherEnabled ? "ON" : "OFF",
                cfg.weatherDisplayName.c_str(), cfg.weatherLat, cfg.weatherLon);
  Serial.printf("[CONFIG] Tempo cache: aujourd'hui=%s demain=%s\n",
                tempoTodayDate.c_str(), tempoTomorrowDate.c_str());
}

void saveConfig() {
  prefs.begin("ztempo", false);

  prefs.putString("hostname", cfg.hostname);
  prefs.putString("shelly", cfg.shellyIp);
  prefs.putBool("shmono", cfg.shellyMonophase);
  prefs.putUChar("shgrid", cfg.shellyGridChannel);
  prefs.putChar("shrout", cfg.shellyRoutedChannel);
  prefs.putChar("shaux", cfg.shellyAuxChannel);
  prefs.putString("shauxlbl", cfg.shellyAuxLabel);
  prefs.putString("f1atb", cfg.f1atbIp);
  prefs.putBool("f1en", cfg.f1atbEnabled);
  prefs.putString("f1lbl", cfg.f1atbActionLabel);
  prefs.putUChar("f1act", cfg.f1atbActionNumber);

  prefs.putBool("wen", cfg.weatherEnabled);
  prefs.putString("wcity", cfg.weatherCity);
  prefs.putString("wdisp", cfg.weatherDisplayName);
  prefs.putDouble("wlat", cfg.weatherLat);
  prefs.putDouble("wlon", cfg.weatherLon);
  prefs.putString("ssid", cfg.wifiSsid);
  prefs.putString("wpass", cfg.wifiPass);
  prefs.putBool("wstatic", cfg.wifiStaticEnabled);
  prefs.putString("wip", cfg.wifiStaticIp);
  prefs.putString("wgw", cfg.wifiGateway);
  prefs.putString("wsub", cfg.wifiSubnet);
  prefs.putString("wdns1", cfg.wifiDns1);
  prefs.putString("wdns2", cfg.wifiDns2);
  prefs.putString("apass", cfg.adminPass);
  prefs.putUShort("chargeW", cfg.totalChargeW);
  prefs.putBool("zwrite", cfg.zendureWritesEnabled);

  prefs.putUChar("zcount", static_cast<uint8_t>(zendureCount));
  for (size_t i = 0; i < MAX_ZENDURE_DEVICES; ++i) {
    const String k = "z" + String(i);
    if (i < zendureCount) {
      prefs.putBool((k + "en").c_str(), zendures[i].enabled);
      prefs.putString((k + "lbl").c_str(), zendures[i].label);
      prefs.putString((k + "sn").c_str(), zendures[i].sn);
      prefs.putString((k + "ip").c_str(), zendures[i].manualIp);
    } else {
      prefs.remove((k + "en").c_str());
      prefs.remove((k + "lbl").c_str());
      prefs.remove((k + "sn").c_str());
      prefs.remove((k + "ip").c_str());
    }
  }

  prefs.end();
  Serial.println("[CONFIG] Configuration sauvegardee en NVS");
}

void cacheTempo() {
  prefs.begin("ztempo", false);

  prefs.putUChar("tToday", static_cast<uint8_t>(tempoToday));
  prefs.putUChar("tTomorrow", static_cast<uint8_t>(tempoTomorrow));
  prefs.putString("tTodayD", tempoTodayDate);
  prefs.putString("tTomD", tempoTomorrowDate);

  prefs.end();
}

void saveShellyEnergyBaselines() {
  prefs.begin("ztempo", false);
  prefs.putString("shday", shellyEnergyDayDate);
  prefs.putFloat("shgib", shellyGridImportDayBaseWh);
  prefs.putFloat("shgeb", shellyGridExportDayBaseWh);
  prefs.putFloat("shrb", shellyRoutedDayBaseWh);
  prefs.putFloat("shauxb", shellyAuxDayBaseWh);
  prefs.end();
}


void savePvEnergyCounters() {
  prefs.begin("ztempo", false);
  prefs.putDouble("pvtot", pvEnergyTotalWh);
  prefs.putDouble("pvbase", pvEnergyDayBaseWh);
  prefs.putString("pvday", pvEnergyDayDate);
  prefs.end();
}
