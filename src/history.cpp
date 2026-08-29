#include <math.h>
#include <limits.h>
#include "history.h"
#include "app_state.h"
#include "settings.h"

static int16_t p16(int32_t v) {
  if (v > INT16_MAX) return INT16_MAX;
  if (v < INT16_MIN) return INT16_MIN;
  return static_cast<int16_t>(v);
}

void addHistory() {
  int32_t pvW = 0;
  int32_t homeFeedW = 0;
  int32_t batteryDischargeW = 0;
  int32_t batteryChargeW = 0;

  for (size_t i = 0; i < zendureCount; ++i) {
    const ZendureDevice& z = zendures[i];
    if (!z.configured || !z.enabled || !z.online) continue;

    pvW += max(0, z.solarInputPower);
    homeFeedW += max(0, z.outputHomePower);
    batteryDischargeW += max(0, z.packInputPower);
    batteryChargeW += max(0, z.outputPackPower);
  }

  const int32_t gridW =
    isnan(shellyPowerW) ? 0 : lroundf(shellyPowerW);

  // "From Grid" = direct Shelly grid-channel import.
  // Negative Shelly values are export, therefore not part of From Grid.
  const int32_t fromGridW = max<int32_t>(0, gridW);

  const int32_t routedW =
    (
      cfg.shellyMonophase &&
      cfg.shellyRoutedChannel >= 0 &&
      !isnan(shellyRoutedPowerW)
    )
      ? max<int32_t>(0, lroundf(shellyRoutedPowerW))
      : 0;

  // Total house load: router included.
  const int32_t houseW = max<int32_t>(0, gridW + homeFeedW);

  // + = battery is supplying the house, - = battery is charging.
  const int32_t batteryW = batteryDischargeW - batteryChargeW;

  // Software PV energy counter from aggregate Zendure PV power.
  // Persisted periodically; day rollover follows local civil midnight.
  static uint32_t lastPvEnergyMs = 0;
  static uint8_t pvSaveDivider = 0;
  const uint32_t nowMs = millis();

  if (lastPvEnergyMs != 0) {
    const uint32_t dtMs = nowMs - lastPvEnergyMs;
    if (dtMs <= 120000UL) {
      pvEnergyTotalWh += (static_cast<double>(pvW) * static_cast<double>(dtMs)) / 3600000.0;
    }
  }
  lastPvEnergyMs = nowMs;

  struct tm localTm;
  if (localTimeSafe(localTm)) {
    char dayBuf[11];
    snprintf(dayBuf, sizeof(dayBuf), "%04d-%02d-%02d",
      localTm.tm_year + 1900, localTm.tm_mon + 1, localTm.tm_mday);
    const String day(dayBuf);

    if (pvEnergyDayDate != day) {
      pvEnergyDayDate = day;
      pvEnergyDayBaseWh = pvEnergyTotalWh;
      pvEnergyTodayWh = 0.0;
      savePvEnergyCounters();
      pvSaveDivider = 0;
    } else {
      pvEnergyTodayWh = max(0.0, pvEnergyTotalWh - pvEnergyDayBaseWh);
      if (++pvSaveDivider >= 30) { // ~15 min with 30 s history cadence
        pvSaveDivider = 0;
        savePvEnergyCounters();
      }
    }
  }

  HistoryPoint& h = historyBuf[historyHead];
  h.epoch = nowEpoch();
  h.pvW = p16(pvW);
  h.houseW = p16(houseW);
  h.batteryW = p16(batteryW);
  h.routedW = p16(routedW);
  h.gridStoreW = p16(fromGridW);

  historyHead = (historyHead + 1) % HISTORY_POINTS;
  if (historyCount < HISTORY_POINTS) ++historyCount;
}
