#include "controller.h"
#include "app_state.h"
#include "zendure.h"
#include "tempo.h"

TempoColor prechargeTargetColor() {
  struct tm t;
  if (!localTimeSafe(t)) {
    return TempoColor::UNKNOWN;
  }

  // Tempo day = 06:00 -> 06:00.
  // During the whole precharge window 22:00 -> 06:00, use J+1.
  // If the API reports J+1 as NON_DEFINI, tempoTomorrow is UNKNOWN and
  // the fail-safe behavior is to NOT force grid charging.
  if (t.tm_hour >= 22 || t.tm_hour < 6) {
    return tempoTomorrowFresh()
      ? tempoTomorrow
      : TempoColor::UNKNOWN;
  }

  return TempoColor::UNKNOWN;
}

const char* tempoWindowName() {
  struct tm t;
  if (!localTimeSafe(t)) return "--";

  if (t.tm_hour >= 22 || t.tm_hour < 6) {
    return "HC 22:00-06:00 · cible demain";
  }

  return "HP 06:00-22:00";
}

bool shouldAutoForceCharge() {
  return prechargeTargetColor() == TempoColor::RED;
}

ControlMode effectiveMode() {
  if (requestedMode != ControlMode::AUTO) {
    // Local monotonic timeout is authoritative and works without NTP/Internet.
    if (
      manualUntilMs != 0 &&
      static_cast<int32_t>(millis() - manualUntilMs) >= 0
    ) {
      requestedMode = ControlMode::AUTO;
      manualUntilEpoch = 0;
      manualUntilMs = 0;
    } else {
      return requestedMode;
    }
  }

  return shouldAutoForceCharge()
    ? ControlMode::FORCE_CHARGE
    : ControlMode::SELF_CONSUMPTION;
}

void evaluateControl(bool forceWrite) {
  const ControlMode desired = effectiveMode();

  // Global dry-run mode: no physical reconciliation, no pending work.
  // The logical applied mode follows the requested/automatic mode only.
  if (!cfg.zendureWritesEnabled) {
    for (size_t i = 0; i < zendureCount; ++i) {
      zendures[i].releasePending = false;
    }
    appliedMode = desired;
    return;
  }

  if (desired == ControlMode::FORCE_CHARGE) {
    // releasePending is intentionally NOT cleared here. Each device that gets
    // a forced-charge write keeps that flag until a later successful release
    // to HEMS, even if only part of the fleet accepted the force command.
    const bool stateMatches =
      zendureStateMatches(desired, cfg.totalChargeW);

    if (!forceWrite && desired == appliedMode && stateMatches) {
      return;
    }

    if (writeForceCharge(cfg.totalChargeW)) {
      appliedMode = desired;
    }

    return;
  }

  // Enter HEMS/autoconsumption by marking every controlled device pending once.
  // Afterwards only pending devices are touched. Offline units keep their flag
  // and receive a single release when they come back online.
  if (forceWrite || appliedMode != ControlMode::SELF_CONSUMPTION) {
    markZendureReleasePending();
    appliedMode = ControlMode::SELF_CONSUMPTION;
  }

  processZendureReleasePending();
}
