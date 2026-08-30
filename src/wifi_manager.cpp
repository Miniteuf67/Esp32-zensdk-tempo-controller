#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

#include "wifi_manager.h"
#include "app_state.h"
#include "config.h"

static DNSServer dnsServer;
static bool setupPortal = false;
static String setupScanHtmlCache;
static String setupScanJsonCache = R"({"scan_ok":false,"count":0,"networks":[]})";

static String htmlEscape(const String& in) {
  String out;
  out.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in[i];
    switch (c) {
      case '&': out += F("&amp;"); break;
      case '<': out += F("&lt;"); break;
      case '>': out += F("&gt;"); break;
      case '"': out += F("&quot;"); break;
      case '\'': out += F("&#39;"); break;
      default: out += c; break;
    }
  }
  return out;
}

void refreshSetupWifiScanCache() {
  // Le scan peut être appelé avant ou après le démarrage du SoftAP. Il est
  // volontairement synchrone ici: on veut disposer d'une liste prête à servir
  // même avec un navigateur captif qui n'exécute pas correctement le JS.
  WiFi.scanDelete();
  delay(60);
  int n = WiFi.scanNetworks(false, true);

  JsonDocument d;
  JsonArray networks = d["networks"].to<JsonArray>();
  String html;

  if (n > 0) {
    for (int i = 0; i < n && i < 25; ++i) {
      const String ssid = WiFi.SSID(i);
      if (ssid.isEmpty()) continue;

      bool duplicate = false;
      for (JsonObject existing : networks) {
        if (String(existing["ssid"] | "") == ssid) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) continue;

      const int32_t rssi = WiFi.RSSI(i);
      const bool open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
      JsonObject o = networks.add<JsonObject>();
      o["ssid"] = ssid;
      o["rssi"] = rssi;
      o["open"] = open;

      const String esc = htmlEscape(ssid);
      html += F("<button type=\"button\" class=\"net\" data-s=\"");
      html += esc;
      html += F("\">");
      html += esc;
      html += F(" &bull; ");
      html += String(rssi);
      html += F(" dBm");
      if (open) html += F(" &bull; ouvert");
      html += F("</button>");
    }
  }

  d["scan_ok"] = n >= 0;
  d["count"] = networks.size();
  if (n < 0) d["error"] = n;

  setupScanHtmlCache = html;
  setupScanJsonCache = "";
  serializeJson(d, setupScanJsonCache);
  WiFi.scanDelete();

  Serial.printf("[WIFI] Scan portail: code=%d, %u reseau(x) memorise(s)\n",
                n, static_cast<unsigned>(networks.size()));
}

String setupWifiScanHtml() {
  return setupScanHtmlCache;
}

String setupWifiScanJson() {
  return setupScanJsonCache;
}

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

  // Stopper toute reconnexion puis scanner AVANT de démarrer le SoftAP. C'est
  // beaucoup plus fiable sur ESP32 qu'un premier scan lancé par le navigateur
  // une fois le portail captif déjà ouvert.
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_STA);
  delay(120);
  refreshSetupWifiScanCache();

  WiFi.mode(WIFI_AP_STA);
  delay(80);
  WiFi.softAP(SETUP_AP_SSID);

  const IPAddress apIp = WiFi.softAPIP();
  dnsServer.start(53, "*", apIp);
  Serial.printf("[WIFI] Portail secours actif: SSID=%s IP=%s\n",
                SETUP_AP_SSID, apIp.toString().c_str());
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
