#include <time.h>
#include "app_state.h"
#include "config.h"

AppConfig cfg;

ZendureDevice zendures[MAX_ZENDURE_DEVICES];
size_t zendureCount = 0;

TempoColor tempoToday = TempoColor::UNKNOWN;
TempoColor tempoTomorrow = TempoColor::UNKNOWN;
String tempoTodayDate;
String tempoTomorrowDate;
uint32_t tempoLastOkMs = 0;

TempoColor tempoNowColor = TempoColor::UNKNOWN;
String tempoNowTariff;
int tempoNowScheduleCode = -1;
uint32_t tempoNowLastOkMs = 0;


float shellyPowerW = NAN;
float shellyVoltageV = NAN;
float shellyCurrentA = NAN;
float shellyPf = NAN;
int shellyRssi = -127;
uint32_t shellyLastOkMs = 0;
float shellyRoutedPowerW = NAN;
float shellyRoutedEnergyWh = NAN;
uint32_t shellyRoutedLastOkMs = 0;
float shellyGridImportTotalWh = NAN;
float shellyGridExportTotalWh = NAN;
float shellyGridImportTodayWh = NAN;
float shellyGridExportTodayWh = NAN;
float shellyRoutedTotalWh = NAN;
float shellyRoutedTodayWh = NAN;
float shellyAuxPowerW = NAN;
float shellyAuxTotalWh = NAN;
float shellyAuxTodayWh = NAN;
uint32_t shellyAuxLastOkMs = 0;
String shellyEnergyDayDate;
double pvEnergyTotalWh = 0.0;
double pvEnergyTodayWh = 0.0;
double pvEnergyDayBaseWh = 0.0;
String pvEnergyDayDate;
float shellyGridImportDayBaseWh = NAN;
float shellyGridExportDayBaseWh = NAN;
float shellyRoutedDayBaseWh = NAN;
float shellyAuxDayBaseWh = NAN;

String f1atbTempo;
uint32_t f1atbLastOkMs = 0;
uint32_t f1atbLastPollMs = 0;
bool f1atbOnline = false;
String f1atbActionDetectedName;
String f1atbActionState;
bool f1atbActionOnline = false;
int f1atbActionForceMinutes = 0;

bool weatherOnline = false;
float weatherTempC = NAN;
float weatherFeelsC = NAN;
float weatherHumidity = NAN;
float weatherWindKmh = NAN;
float weatherPrecipMm = NAN;
int weatherCode = -1;
float weatherTomorrowMinC = NAN;
float weatherTomorrowMaxC = NAN;
int weatherTomorrowCode = -1;
uint32_t weatherLastOkMs = 0;

ControlMode requestedMode = ControlMode::AUTO;
uint32_t manualUntilEpoch = 0;
uint32_t manualUntilMs = 0;
ControlMode appliedMode = ControlMode::SELF_CONSUMPTION;

HistoryPoint historyBuf[HISTORY_POINTS];
size_t historyHead = 0;
size_t historyCount = 0;

uint32_t nowEpoch() {
  time_t now;
  time(&now);
  if (now < 1700000000) return 0;
  return static_cast<uint32_t>(now);
}

bool localTimeSafe(struct tm& t) {
  time_t now;
  time(&now);
  if (now < 1700000000) return false;
  localtime_r(&now, &t);
  return true;
}

const char* tempoColorName(TempoColor c) {
  switch (c) {
    case TempoColor::BLUE: return "Bleu";
    case TempoColor::WHITE: return "Blanc";
    case TempoColor::RED: return "Rouge";
    default: return "Inconnu";
  }
}
