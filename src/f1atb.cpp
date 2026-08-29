#include <WiFi.h>
#include <HTTPClient.h>

#include "f1atb.h"
#include "app_state.h"

static bool httpGetText(const String& url, String& body, int timeoutMs = 2500) {
  WiFiClient client;
  HTTPClient http;

  http.setConnectTimeout(timeoutMs);
  http.setTimeout(timeoutMs);

  if (!http.begin(client, url)) return false;

  const int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }

  body = http.getString();
  http.end();
  return true;
}

static void parseTempo(const String& body) {
  const int start = body.indexOf("TEMPO_");
  if (start < 0) return;

  int end = start;
  while (
    end < static_cast<int>(body.length()) &&
    body[end] != '\x1e' && body[end] != '\x1d' && body[end] != '\x1c' &&
    body[end] != '|' && body[end] != '\r' && body[end] != '\n'
  ) {
    ++end;
  }

  f1atbTempo = body.substring(start, end);
}

static String cleanField(String s) {
  s.trim();
  while (s.length() && static_cast<uint8_t>(s[0]) < 32) s.remove(0, 1);
  while (s.length() && static_cast<uint8_t>(s[s.length() - 1]) < 32) s.remove(s.length() - 1, 1);
  s.trim();
  return s;
}

static bool isIntegerField(const String& s) {
  if (s.isEmpty()) return false;
  int i = 0;
  if (s[0] == '-' || s[0] == '+') i = 1;
  if (i >= static_cast<int>(s.length())) return false;
  for (; i < static_cast<int>(s.length()); ++i) {
    if (!isDigit(static_cast<unsigned char>(s[i]))) return false;
  }
  return true;
}

static void logEscapedBody(const char* label, const String& body) {
  Serial.printf("[F1ATB] %s brut %u octets: ", label, static_cast<unsigned>(body.length()));
  const size_t maxLen = body.length() > 420 ? 420 : body.length();
  for (size_t i = 0; i < maxLen; ++i) {
    const uint8_t c = static_cast<uint8_t>(body[i]);
    if (c == 0x1D) Serial.print("<GS>");
    else if (c == 0x1E) Serial.print("<RS>");
    else if (c == 0x1F) Serial.print("<US>");
    else if (c == '\r') Serial.print("<CR>");
    else if (c == '\n') Serial.print("<LF>");
    else if (c >= 32 && c <= 126) Serial.write(c);
    else Serial.printf("<%02X>", c);
  }
  if (body.length() > maxLen) Serial.print("...");
  Serial.println();
}

// Parse /ajax_etatActions without assuming a fixed block offset. F1ATB uses
// GS (0x1D) between blocks and RS (0x1E) between fields, but the preamble has
// changed between firmware revisions. We therefore scan every block and only
// accept a block whose first field is exactly the configured NumAction.
static bool parseActionState(const String& body) {
  f1atbActionOnline = false;
  f1atbActionDetectedName = "";
  f1atbActionState = "";
  f1atbActionForceMinutes = 0;

  int blockStart = 0;
  while (blockStart <= static_cast<int>(body.length())) {
    int blockEnd = body.indexOf('\x1d', blockStart);
    if (blockEnd < 0) blockEnd = body.length();
    const String block = body.substring(blockStart, blockEnd);

    String fields[8];
    int fieldCount = 0;
    int fieldStart = 0;
    while (fieldStart <= static_cast<int>(block.length()) && fieldCount < 8) {
      int fieldEnd = block.indexOf('\x1e', fieldStart);
      if (fieldEnd < 0) fieldEnd = block.length();
      fields[fieldCount++] = cleanField(block.substring(fieldStart, fieldEnd));
      if (fieldEnd >= static_cast<int>(block.length())) break;
      fieldStart = fieldEnd + 1;
    }

    if (fieldCount >= 3 && isIntegerField(fields[0])) {
      const int actionNumber = fields[0].toInt();
      if (actionNumber == cfg.f1atbActionNumber) {
        f1atbActionDetectedName = fields[1];
        f1atbActionState = fields[2];
        // F1ATB V17.29b: data[3] = forçage courant en minutes.
        // Positif = forçage ON, négatif = forçage OFF, zéro = aucun.
        // Cette lecture est indexée par NumAction : elle fonctionne donc pour
        // le triac (0), Relais 1 (1), Relais 2 (2), etc.
        f1atbActionForceMinutes =
          (fieldCount >= 4 && isIntegerField(fields[3])) ? fields[3].toInt() : 0;
        f1atbActionOnline = true;
        return true;
      }
    }

    if (blockEnd >= static_cast<int>(body.length())) break;
    blockStart = blockEnd + 1;
  }

  return false;
}

// Fallback valid on recent F1ATB releases: query one action directly.
// Response fields are separated by GS: Actif / Ouvre / Hequiv.  This endpoint
// does not expose the title, so the configured label is used when necessary.
static bool parseActionSpecific(const String& body) {
  String tokens[8];
  int count = 0;
  String cur;

  for (size_t i = 0; i <= body.length() && count < 8; ++i) {
    const uint8_t c = (i < body.length()) ? static_cast<uint8_t>(body[i]) : 0x1D;
    if (c == 0x1D || c == 0x1E || c == 0x1F || c == '\r' || c == '\n') {
      String t = cleanField(cur);
      if (!t.isEmpty()) tokens[count++] = t;
      cur = "";
    } else {
      cur += static_cast<char>(c);
    }
  }

  // Find the first numeric triplet. Some versions prepend an empty GS.
  for (int i = 0; i + 2 < count; ++i) {
    if (!isIntegerField(tokens[i])) continue;
    if (!isIntegerField(tokens[i + 1])) continue;

    const int activeType = tokens[i].toInt();
    const int opening = tokens[i + 1].toInt();
    if (activeType < 0 || activeType > 20 || opening < 0 || opening > 100) continue;

    f1atbActionOnline = true;
    if (f1atbActionDetectedName.isEmpty()) f1atbActionDetectedName = cfg.f1atbActionLabel;

    if (activeType == 0) {
      f1atbActionState = "Inactif";
    } else if (opening <= 0) {
      f1atbActionState = "Off (0 %)";
    } else if (opening >= 100) {
      f1atbActionState = "On (100 %)";
    } else {
      f1atbActionState = String(opening) + " %";
    }
    return true;
  }

  return false;
}

static bool fetchConfiguredAction(String* rawGlobal = nullptr, String* rawSpecific = nullptr) {
  String bodyGlobal;
  const bool globalHttpOk = httpGetText("http://" + cfg.f1atbIp + "/ajax_etatActions", bodyGlobal, 2000);
  if (rawGlobal) *rawGlobal = bodyGlobal;
  if (globalHttpOk && parseActionState(bodyGlobal)) return true;

  String bodySpecific;
  const String specificUrl =
    "http://" + cfg.f1atbIp + "/ajax_etatActionX?NumAction=" + String(cfg.f1atbActionNumber);
  const bool specificHttpOk = httpGetText(specificUrl, bodySpecific, 2000);
  if (rawSpecific) *rawSpecific = bodySpecific;
  if (specificHttpOk && parseActionSpecific(bodySpecific)) return true;

  f1atbActionOnline = false;
  return false;
}

void pollF1ATB() {
  if (!cfg.f1atbEnabled || cfg.f1atbIp.isEmpty()) {
    f1atbTempo = "";
    f1atbActionDetectedName = "";
    f1atbActionState = "";
    f1atbActionOnline = false;
    f1atbActionForceMinutes = 0;
    f1atbOnline = false;
    f1atbLastOkMs = 0;
    f1atbLastPollMs = 0;
    return;
  }

  f1atbLastPollMs = millis();
  const bool wasOnline = f1atbOnline;
  const bool wasActionOnline = f1atbActionOnline;
  const String oldActionState = f1atbActionState;
  const String oldDetectedName = f1atbActionDetectedName;
  const int oldForceMinutes = f1atbActionForceMinutes;

  String dataBody;
  const bool dataOk = httpGetText("http://" + cfg.f1atbIp + "/ajax_data", dataBody, 2000);
  if (dataOk) parseTempo(dataBody);

  String rawGlobal;
  String rawSpecific;
  const bool actionFound = fetchConfiguredAction(&rawGlobal, &rawSpecific);
  const bool actionEndpointReachable = !rawGlobal.isEmpty() || !rawSpecific.isEmpty();

  // The controller itself is considered reachable if ajax_data works or an
  // action endpoint returned a body. Synchronisation requires a parsed action.
  f1atbOnline = dataOk || actionEndpointReachable;
  if (f1atbOnline) f1atbLastOkMs = millis();

  const bool changed =
    (wasOnline != f1atbOnline) ||
    (wasActionOnline != f1atbActionOnline) ||
    (oldActionState != f1atbActionState) ||
    (oldDetectedName != f1atbActionDetectedName) ||
    (oldForceMinutes != f1atbActionForceMinutes);

  static uint32_t lastHeartbeatLogMs = 0;
  static bool loggedOnce = false;
  static bool rawFailureLogged = false;
  const bool heartbeat = millis() - lastHeartbeatLogMs >= 60000UL;

  if (!actionFound && !rawFailureLogged) {
    if (!rawGlobal.isEmpty()) logEscapedBody("ajax_etatActions", rawGlobal);
    if (!rawSpecific.isEmpty()) logEscapedBody("ajax_etatActionX", rawSpecific);
    rawFailureLogged = true;
  }
  if (actionFound) rawFailureLogged = false;

  if (!loggedOnce || changed || heartbeat) {
    Serial.printf(
      "[F1ATB] %s IP=%s ajax_data=%s action#%u=%s nom='%s' etat='%s' force=%+dmin\n",
      f1atbOnline ? "ONLINE" : "OFFLINE",
      cfg.f1atbIp.c_str(),
      dataOk ? "OK" : "ECHEC",
      static_cast<unsigned>(cfg.f1atbActionNumber),
      f1atbActionOnline ? "SYNC" : "NON_SYNC",
      f1atbActionDetectedName.c_str(),
      f1atbActionState.c_str(),
      f1atbActionForceMinutes
    );
    lastHeartbeatLogMs = millis();
    loggedOnce = true;
  }
}

static bool forceF1ATB(int forceMinutes) {
  if (!cfg.f1atbEnabled || cfg.f1atbIp.isEmpty()) return false;

  // F1ATB V17.29b: Force is expressed directly in minutes.
  // +30 = force ON, -30 = force OFF, 0 = cancel forcing / return to AUTO.
  String body;

  const String url =
    "http://" + cfg.f1atbIp +
    "/ForceAction?Force=" + String(forceMinutes) +
    "&NumAction=" + String(cfg.f1atbActionNumber);

  const char* label = forceMinutes > 0 ? "ON +30 min" : (forceMinutes < 0 ? "OFF -30 min" : "ANNULER / AUTO");
  Serial.printf(
    "[F1ATB] Commande %s action#%u via /ForceAction -> %s\n",
    label,
    static_cast<unsigned>(cfg.f1atbActionNumber),
    cfg.f1atbIp.c_str()
  );

  const bool ok = httpGetText(url, body, 2500);
  f1atbLastPollMs = millis();

  if (ok) {
    if (!body.isEmpty()) {
      String preview = body;
      preview.replace("\r", " ");
      preview.replace("\n", " ");
      if (preview.length() > 240) preview = preview.substring(0, 240) + "...";
      Serial.printf("[F1ATB] Reponse ForceAction='%s'\n", preview.c_str());
    }

    // Let F1ATB commit the forcing state before reading it back.
    delay(800);
    const bool actionFound = fetchConfiguredAction();
    f1atbOnline = true;
    f1atbLastOkMs = millis();
    Serial.printf(
      "[F1ATB] Commande OK action=%s etat='%s' force=%+dmin\n",
      actionFound ? "SYNC" : "INTROUVABLE",
      f1atbActionState.c_str(),
      f1atbActionForceMinutes
    );
  } else {
    f1atbOnline = false;
    f1atbActionOnline = false;
    Serial.println("[F1ATB] Commande ECHEC: routeur injoignable");
  }

  return ok;
}

bool stepF1ATBOn() { return forceF1ATB(30); }
bool stepF1ATBOff() { return forceF1ATB(-30); }
bool cancelF1ATBForce() { return forceF1ATB(0); }
