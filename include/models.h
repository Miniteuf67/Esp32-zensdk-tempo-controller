#pragma once
#include <Arduino.h>

enum class TempoColor : uint8_t {
  UNKNOWN = 0,
  BLUE = 1,
  WHITE = 2,
  RED = 3
};

enum class ControlMode : uint8_t {
  AUTO = 0,
  SELF_CONSUMPTION = 1,
  FORCE_CHARGE = 2
};

struct AppConfig {
  String hostname;
  String shellyIp;
  bool shellyMonophase = true;
  uint8_t shellyGridChannel = 0;
  int8_t shellyRoutedChannel = 1; // -1 = disabled; monophase only
  int8_t shellyAuxChannel = -1; // -1 = disabled; monophase only
  String shellyAuxLabel = "Canal personnalisé";
  String f1atbIp;
  bool f1atbEnabled = false;
  String f1atbActionLabel = "Chauffe-eau";
  uint8_t f1atbActionNumber = 0;

  bool weatherEnabled = false;
  String weatherCity;
  String weatherDisplayName;
  double weatherLat = 0.0;
  double weatherLon = 0.0;

  String wifiSsid;
  String wifiPass;
  bool wifiStaticEnabled = false;
  String wifiStaticIp;
  String wifiGateway;
  String wifiSubnet = "255.255.255.0";
  String wifiDns1;
  String wifiDns2;
  String adminPass;
  uint16_t totalChargeW = 2400;
  bool zendureWritesEnabled = false;
};

struct ZendureDevice {
  // Persistent configuration
  bool configured = false;
  bool enabled = true;
  String label;
  String sn;
  String manualIp;

  // Runtime discovery/state
  String name;
  String ip;
  uint16_t port = 80;

  bool online = false;
  int soc = -1;
  int gridInputPower = -1;
  int solarInputPower = -1;
  int outputHomePower = -1;
  int packInputPower = -1;   // battery discharge power
  int outputPackPower = -1;  // battery charge power
  int inputLimit = -1;
  int outputLimit = -1;
  int acMode = -1;
  int smartMode = -1;
  uint32_t lastSeenMs = 0;
  bool releasePending = false; // runtime only: release to HEMS when reachable
};

struct HistoryPoint {
  uint32_t epoch = 0;
  int16_t pvW = 0;
  int16_t houseW = 0;
  int16_t batteryW = 0; // + discharge / - charge
  int16_t routedW = 0;
  int16_t gridStoreW = 0; // AC grid -> battery storage
};
