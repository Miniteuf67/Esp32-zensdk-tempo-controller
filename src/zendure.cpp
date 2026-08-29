#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include "zendure.h"
#include "app_state.h"
#include "config.h"
#include "http_helpers.h"

static bool zendureWrite(ZendureDevice& device, JsonObject props) {
  if (!cfg.zendureWritesEnabled) return false;
  if (!device.online || device.sn.isEmpty()) return false;

  WiFiClient client;
  HTTPClient http;

  http.setConnectTimeout(2500);
  http.setTimeout(3000);

  const String url = "http://" + device.ip + ":" + String(device.port) + "/properties/write";
  if (!http.begin(client, url)) return false;

  http.addHeader("Content-Type", "application/json");

  JsonDocument payload;
  payload["sn"] = device.sn;
  payload["properties"] = props;

  String body;
  serializeJson(payload, body);

  const int code = http.POST(body);
  http.end();

  return code >= 200 && code < 300;
}

void discoverZendure() {
  // Start from configured manual IPs so a device remains usable even if mDNS fails.
  for (size_t i = 0; i < zendureCount; ++i) {
    if (!zendures[i].configured || !zendures[i].enabled) continue;
    if (!zendures[i].manualIp.isEmpty()) {
      zendures[i].ip = zendures[i].manualIp;
      zendures[i].port = 80;
    }
  }

  const int foundCount = MDNS.queryService("zendure", "tcp");
  if (foundCount <= 0) return;

  for (int n = 0; n < foundCount; ++n) {
    const String host = MDNS.hostname(n);
    const String ip = MDNS.IP(n).toString();
    const uint16_t port = MDNS.port(n) ? MDNS.port(n) : 80;
    if (ip.isEmpty()) continue;

    // Read the report to obtain the authoritative serial number.
    JsonDocument doc;
    const String url = "http://" + ip + ":" + String(port) + "/properties/report";
    if (!httpGetJson(url, doc, 1800)) continue;

    JsonObject props = doc["properties"];
    if (props.isNull()) props = doc.as<JsonObject>();

    String discoveredSn = doc["sn"] | "";
    if (discoveredSn.isEmpty()) discoveredSn = props["sn"] | "";

    // Some firmware versions expose the serial in the mDNS hostname.
    if (discoveredSn.isEmpty()) {
      for (size_t i = 0; i < zendureCount; ++i) {
        if (!zendures[i].sn.isEmpty() && host.indexOf(zendures[i].sn) >= 0) {
          discoveredSn = zendures[i].sn;
          break;
        }
      }
    }

    if (discoveredSn.isEmpty()) continue;

    for (size_t i = 0; i < zendureCount; ++i) {
      if (!zendures[i].configured || !zendures[i].enabled) continue;
      if (!zendures[i].sn.equalsIgnoreCase(discoveredSn)) continue;

      zendures[i].name = host;
      zendures[i].ip = ip;
      zendures[i].port = port;
      zendures[i].online = true;
      zendures[i].lastSeenMs = millis();
      break;
    }
  }
}

void pollZendures() {
  for (size_t i = 0; i < zendureCount; ++i) {
    if (!zendures[i].configured || !zendures[i].enabled || zendures[i].ip.isEmpty()) {
      zendures[i].online = false;
      continue;
    }

    JsonDocument doc;

    const String url =
      "http://" + zendures[i].ip + ":" + String(zendures[i].port) + "/properties/report";

    if (!httpGetJson(url, doc, 3000)) {
      zendures[i].online = false;
      continue;
    }

    JsonObject props = doc["properties"];
    if (props.isNull()) props = doc.as<JsonObject>();

    zendures[i].sn = doc["sn"] | zendures[i].sn;
    if (zendures[i].sn.isEmpty()) {
      zendures[i].sn = props["sn"] | "";
    }

    if (zendures[i].sn.isEmpty()) {
      const int pos = zendures[i].name.lastIndexOf('-');
      if (pos >= 0) {
        zendures[i].sn = zendures[i].name.substring(pos + 1);
      }
    }

    zendures[i].soc = props["electricLevel"] | -1;
    zendures[i].gridInputPower = props["gridInputPower"] | -1;
    zendures[i].solarInputPower = props["solarInputPower"] | -1;
    zendures[i].outputHomePower = props["outputHomePower"] | -1;
    zendures[i].packInputPower = props["packInputPower"] | -1;
    zendures[i].outputPackPower = props["outputPackPower"] | -1;
    zendures[i].inputLimit = props["inputLimit"] | -1;
    zendures[i].outputLimit = props["outputLimit"] | -1;
    zendures[i].acMode = props["acMode"] | -1;
    zendures[i].smartMode = props["smartMode"] | -1;

    zendures[i].online = true;
    zendures[i].lastSeenMs = millis();
  }
}

bool writeForceCharge(uint16_t totalW) {
  // When physical writes are disabled, control is intentionally dry-run.
  if (!cfg.zendureWritesEnabled) return true;

  size_t activeCount = 0;

  for (size_t i = 0; i < zendureCount; ++i) {
    if (zendures[i].configured && zendures[i].enabled) {
      ++activeCount;
    }
  }

  if (activeCount == 0) return true;

  const uint16_t each =
    min<uint16_t>(
      MAX_PER_DEVICE_CHARGE_W,
      (totalW + activeCount - 1) / activeCount
    );

  bool allOk = true;

  for (size_t i = 0; i < zendureCount; ++i) {
    if (!zendures[i].configured || !zendures[i].enabled) continue;

    // An offline configured device means the requested global mode has not
    // been reconciled yet. Keep controller retrying when it comes back.
    if (!zendures[i].online) {
      allOk = false;
      continue;
    }

    JsonDocument doc;
    JsonObject props = doc.to<JsonObject>();

    props["smartMode"] = 1;
    props["acMode"] = 1;
    props["outputLimit"] = 0;
    props["inputLimit"] = each;

    // From the moment we attempt a physical forced-charge write, remember
    // that this device must later receive a release to HEMS. Keep the flag
    // even when the HTTP result is ambiguous/fails: the device may still have
    // accepted the command before the connection failed.
    zendures[i].releasePending = true;

    if (!zendureWrite(zendures[i], props)) {
      allOk = false;
    }

    delay(80);
  }

  return allOk;
}

static bool releaseDeviceToHems(ZendureDevice& device) {
  if (!cfg.zendureWritesEnabled) return false;
  if (!device.configured || !device.enabled) return true;
  if (!device.online || device.ip.isEmpty()) return false;

  JsonDocument doc;
  JsonObject props = doc.to<JsonObject>();

  props["smartMode"] = 0;
  props["acMode"] = 2;
  props["outputLimit"] = 0;
  props["inputLimit"] = 0;

  return zendureWrite(device, props);
}

bool releaseZendureIndexNow(size_t index) {
  if (index >= zendureCount) return false;

  const bool ok = releaseDeviceToHems(zendures[index]);

  if (ok) {
    zendures[index].releasePending = false;
  }

  return ok;
}

void markZendureReleasePending() {
  for (size_t i = 0; i < zendureCount; ++i) {
    if (zendures[i].configured && zendures[i].enabled) {
      zendures[i].releasePending = true;
    }
  }
}

bool hasZendureReleasePending() {
  for (size_t i = 0; i < zendureCount; ++i) {
    if (
      zendures[i].configured &&
      zendures[i].enabled &&
      zendures[i].releasePending
    ) {
      return true;
    }
  }

  return false;
}

bool processZendureReleasePending() {
  if (!cfg.zendureWritesEnabled) {
    return !hasZendureReleasePending();
  }

  bool allCleared = true;

  for (size_t i = 0; i < zendureCount; ++i) {
    ZendureDevice& z = zendures[i];

    if (!z.configured || !z.enabled || !z.releasePending) continue;

    // Offline devices remain pending, but we do not touch already released
    // online devices again. Their release will happen once after they return.
    if (!z.online || z.ip.isEmpty()) {
      allCleared = false;
      continue;
    }

    if (releaseDeviceToHems(z)) {
      z.releasePending = false;
    } else {
      allCleared = false;
    }

    delay(80);
  }

  return allCleared && !hasZendureReleasePending();
}

bool writeReleaseToHems() {
  if (!cfg.zendureWritesEnabled) return true;

  markZendureReleasePending();
  return processZendureReleasePending();
}


bool zendureStateMatches(ControlMode desired, uint16_t totalChargeW) {
  if (desired != ControlMode::FORCE_CHARGE) {
    // HEMS/autoconsumption convergence is tracked explicitly with
    // releasePending. We do not infer ownership from smartMode/acMode because
    // HEMS may legitimately use those properties itself.
    return !hasZendureReleasePending();
  }

  size_t activeCount = 0;

  for (size_t i = 0; i < zendureCount; ++i) {
    if (zendures[i].configured && zendures[i].enabled) {
      ++activeCount;
    }
  }

  if (activeCount == 0) return true;

  const uint16_t expectedEach =
    min<uint16_t>(
      MAX_PER_DEVICE_CHARGE_W,
      (totalChargeW + activeCount - 1) / activeCount
    );

  for (size_t i = 0; i < zendureCount; ++i) {
    const ZendureDevice& z = zendures[i];

    if (!z.configured || !z.enabled) continue;
    if (!z.online) return false;

    if (z.smartMode != 1) return false;
    if (z.acMode != 1) return false;
    if (z.outputLimit != 0) return false;
    if (z.inputLimit != expectedEach) return false;
  }

  return true;
}
