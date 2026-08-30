#pragma once
#include <Arduino.h>

bool connectWifi();
void startSetupPortal();
bool isSetupPortal();
void processSetupDns();

// Scan de secours exécuté côté ESP32. Le résultat est mis en cache afin que
// la page du portail puisse afficher les réseaux même si le JavaScript du
// navigateur captif ne lance pas /api/setup/scan.
void refreshSetupWifiScanCache();
String setupWifiScanHtml();
String setupWifiScanJson();

void startMdns();
