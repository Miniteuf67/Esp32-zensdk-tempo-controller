#pragma once
#include <Arduino.h>
#include "models.h"
#include "config.h"

extern AppConfig cfg;

extern ZendureDevice zendures[MAX_ZENDURE_DEVICES];
extern size_t zendureCount;

extern TempoColor tempoToday;
extern TempoColor tempoTomorrow;
extern String tempoTodayDate;
extern String tempoTomorrowDate;
extern uint32_t tempoLastOkMs;

extern TempoColor tempoNowColor;
extern String tempoNowTariff;
extern int tempoNowScheduleCode;
extern uint32_t tempoNowLastOkMs;


extern float shellyPowerW;
extern float shellyVoltageV;
extern float shellyCurrentA;
extern float shellyPf;
extern int shellyRssi;
extern uint32_t shellyLastOkMs;
extern float shellyRoutedPowerW;
extern float shellyRoutedEnergyWh;
extern uint32_t shellyRoutedLastOkMs;
extern float shellyGridImportTotalWh;
extern float shellyGridExportTotalWh;
extern float shellyGridImportTodayWh;
extern float shellyGridExportTodayWh;
extern float shellyRoutedTotalWh;
extern float shellyRoutedTodayWh;
extern float shellyAuxPowerW;
extern float shellyAuxTotalWh;
extern float shellyAuxTodayWh;
extern uint32_t shellyAuxLastOkMs;
extern String shellyEnergyDayDate;
extern double pvEnergyTotalWh;
extern double pvEnergyTodayWh;
extern double pvEnergyDayBaseWh;
extern String pvEnergyDayDate;
extern float shellyGridImportDayBaseWh;
extern float shellyGridExportDayBaseWh;
extern float shellyRoutedDayBaseWh;
extern float shellyAuxDayBaseWh;

extern String f1atbTempo;
extern uint32_t f1atbLastOkMs;
extern uint32_t f1atbLastPollMs;
extern bool f1atbOnline;

extern String f1atbActionDetectedName;
extern String f1atbActionState;
extern bool f1atbActionOnline;
extern int f1atbActionForceMinutes; // >0 ON, <0 OFF, 0 aucun forcage

extern bool weatherOnline;
extern float weatherTempC;
extern float weatherFeelsC;
extern float weatherHumidity;
extern float weatherWindKmh;
extern float weatherPrecipMm;
extern int weatherCode;
extern float weatherTomorrowMinC;
extern float weatherTomorrowMaxC;
extern int weatherTomorrowCode;
extern uint32_t weatherLastOkMs;

extern ControlMode requestedMode;
extern uint32_t manualUntilEpoch;
extern uint32_t manualUntilMs;
extern ControlMode appliedMode;

extern HistoryPoint historyBuf[HISTORY_POINTS];
extern size_t historyHead;
extern size_t historyCount;

uint32_t nowEpoch();
bool localTimeSafe(struct tm& t);
const char* tempoColorName(TempoColor c);
