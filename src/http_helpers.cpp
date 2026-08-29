#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "http_helpers.h"

static String hostFromUrl(const String& url) {
  int start = url.indexOf("://");
  start = start >= 0 ? start + 3 : 0;
  int end = url.indexOf('/', start);
  if (end < 0) end = url.length();
  String host = url.substring(start, end);
  const int colon = host.indexOf(':');
  if (colon >= 0) host = host.substring(0, colon);
  return host;
}

static String payloadPreview(const String& payload, size_t maxLen = 180) {
  String p = payload.substring(0, min(maxLen, payload.length()));
  p.replace("\r", " ");
  p.replace("\n", " ");
  return p;
}

bool httpGetJson(const String& url, JsonDocument& doc, int timeoutMs) {
  const String host = hostFromUrl(url);
  IPAddress resolved;
  if (!host.isEmpty() && WiFi.hostByName(host.c_str(), resolved) != 1) {
    Serial.printf("[HTTP] DNS ECHEC host=%s DNS1=%s DNS2=%s\n",
                  host.c_str(),
                  WiFi.dnsIP(0).toString().c_str(),
                  WiFi.dnsIP(1).toString().c_str());
    return false;
  }

  const bool https = url.startsWith("https://");
  Serial.printf("[HTTP] GET %s -> %s (%s)\n",
                url.c_str(), resolved.toString().c_str(), https ? "TLS" : "HTTP");

  HTTPClient http;
  http.setConnectTimeout(timeoutMs);
  http.setTimeout(timeoutMs);

  // Force une reponse non compressee et sans chunking ambigu pour ArduinoJson.
  // La v7 deserialisait directement le stream HTTP; Open-Meteo pouvait alors
  // produire InvalidInput malgre une connexion TLS correcte.
  http.useHTTP10(true);

  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  bool beginOk = false;

  if (https) {
    // Même principe que F1ATB pour les services HTTPS publics : client TLS dédié.
    // setInsecure() évite de dépendre d'un certificat CA embarqué susceptible d'expirer.
    secureClient.setInsecure();
    beginOk = http.begin(secureClient, url);
  } else {
    beginOk = http.begin(plainClient, url);
  }

  if (!beginOk) {
    Serial.printf("[HTTP] begin() ECHEC: %s\n", url.c_str());
    return false;
  }

  http.addHeader("Accept", "application/json");
  http.addHeader("Accept-Encoding", "identity");

  const int code = http.GET();
  if (code != 200) {
    const String body = code > 0 ? http.getString() : String();
    Serial.printf("[HTTP] ECHEC code=%d (%s) url=%s\n",
                  code, HTTPClient::errorToString(code).c_str(), url.c_str());
    if (!body.isEmpty()) {
      Serial.printf("[HTTP] BODY: %s\n", payloadPreview(body).c_str());
    }
    http.end();
    return false;
  }

  // Lire d'abord le corps complet puis parser. C'est volontairement plus robuste
  // que deserializeJson(doc, http.getStream()) sur certaines reponses HTTPS ESP32.
  const String payload = http.getString();
  http.end();

  if (payload.isEmpty()) {
    Serial.printf("[HTTP] ECHEC: corps vide url=%s\n", url.c_str());
    return false;
  }

  const DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[HTTP] JSON ECHEC: %s bytes=%u url=%s\n",
                  err.c_str(), static_cast<unsigned>(payload.length()), url.c_str());
    Serial.printf("[HTTP] BODY: %s\n", payloadPreview(payload).c_str());
    return false;
  }

  Serial.printf("[HTTP] OK 200 JSON=%u bytes host=%s\n",
                static_cast<unsigned>(payload.length()), host.c_str());
  return true;
}
