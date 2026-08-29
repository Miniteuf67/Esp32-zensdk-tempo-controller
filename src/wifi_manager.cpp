#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

#include "wifi_manager.h"
#include "app_state.h"
#include "config.h"

static DNSServer dnsServer;
static bool setupPortal = false;

bool connectWifi() {
  if (cfg.wifiSsid.isEmpty()) {
    Serial.println("[WIFI] SSID vide -> portail de configuration");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(cfg.hostname.c_str());

  if (cfg.wifiStaticEnabled) {
    IPAddress ip, gw, sub, dns1, dns2;
    const bool valid =
      ip.fromString(cfg.wifiStaticIp) &&
      gw.fromString(cfg.wifiGateway) &&
      sub.fromString(cfg.wifiSubnet);

    if (valid) {
      const bool hasDns1 = dns1.fromString(cfg.wifiDns1);
      const bool hasDns2 = dns2.fromString(cfg.wifiDns2);

      if (hasDns1 && hasDns2) {
        WiFi.config(ip, gw, sub, dns1, dns2);
      } else if (hasDns1) {
        WiFi.config(ip, gw, sub, dns1);
      } else {
        WiFi.config(ip, gw, sub);
      }
      Serial.printf("[WIFI] IP statique demandee: %s GW=%s DNS1=%s DNS2=%s\n",
                    cfg.wifiStaticIp.c_str(), cfg.wifiGateway.c_str(),
                    cfg.wifiDns1.c_str(), cfg.wifiDns2.c_str());
    } else {
      Serial.println("[WIFI] Configuration IP statique invalide -> connexion sans WiFi.config()");
    }
  } else {
    Serial.println("[WIFI] Mode DHCP");
  }

  WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPass.c_str());

  const uint32_t start = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - start < WIFI_CONNECT_TIMEOUT_MS
  ) {
    delay(250);
  }

  const bool ok = WiFi.status() == WL_CONNECTED;
  if (ok) {
    Serial.printf("[WIFI] Connecte a %s - IP=%s RSSI=%d dBm\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
    Serial.printf("[WIFI] GW=%s DNS1=%s DNS2=%s\n",
                  WiFi.gatewayIP().toString().c_str(),
                  WiFi.dnsIP(0).toString().c_str(),
                  WiFi.dnsIP(1).toString().c_str());
  } else {
    Serial.printf("[WIFI] Echec connexion, status=%d\n", static_cast<int>(WiFi.status()));
  }
  return ok;
}

void startSetupPortal() {
  setupPortal = true;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(SETUP_AP_SSID);

  dnsServer.start(53, "*", WiFi.softAPIP());
}

bool isSetupPortal() {
  return setupPortal;
}

void processSetupDns() {
  if (setupPortal) {
    dnsServer.processNextRequest();
  }
}

void startMdns() {
  if (MDNS.begin(cfg.hostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
  }
}
