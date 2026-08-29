#pragma once
#include <Arduino.h>

bool connectWifi();
void startSetupPortal();
bool isSetupPortal();
void processSetupDns();

void startMdns();
