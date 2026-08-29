#pragma once
#include "models.h"

bool shouldAutoForceCharge();
ControlMode effectiveMode();
void evaluateControl(bool forceWrite = false);

TempoColor prechargeTargetColor();
const char* tempoWindowName();
