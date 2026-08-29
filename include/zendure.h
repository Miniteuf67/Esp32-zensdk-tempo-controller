#pragma once
#include <Arduino.h>
#include "models.h"

void discoverZendure();
void pollZendures();

bool writeForceCharge(uint16_t totalW);
bool writeReleaseToHems();

bool zendureStateMatches(ControlMode desired, uint16_t totalChargeW);

void markZendureReleasePending();
bool processZendureReleasePending();
bool hasZendureReleasePending();
bool releaseZendureIndexNow(size_t index);