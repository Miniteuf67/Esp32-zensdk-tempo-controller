#pragma once
#include <Arduino.h>

static constexpr const char* DEFAULT_HOSTNAME = "zendure-tempo";
static constexpr const char* DEFAULT_SHELLY_IP = "";
static constexpr const char* DEFAULT_F1ATB_IP  = "";

static constexpr const char* TEMPO_HOST = "www.services-rte.com";
static constexpr const char* TEMPO_PATH = "/cms/open_data/v1/tempoLight";

static constexpr uint32_t SHELLY_POLL_MS = 2000;
static constexpr uint32_t ZENDURE_STATUS_POLL_MS = 10000;
static constexpr uint32_t F1ATB_POLL_MS = 5000;
static constexpr uint32_t TEMPO_REFRESH_MS = 15UL * 60UL * 1000UL;
static constexpr uint32_t CONTROL_EVAL_MS = 5000;
static constexpr uint32_t MDNS_DISCOVERY_MS = 60UL * 1000UL;
static constexpr uint32_t HISTORY_SAMPLE_MS = 30000;

static constexpr size_t HISTORY_POINTS = 2880; // 24 h à 30 s

static constexpr uint16_t DEFAULT_TOTAL_CHARGE_W = 2400;
static constexpr uint16_t MAX_TOTAL_CHARGE_W = 2400;
static constexpr uint16_t MAX_PER_DEVICE_CHARGE_W = 800;
static constexpr uint16_t MIN_MANUAL_TIMEOUT_MIN = 1;
static constexpr uint16_t MAX_MANUAL_TIMEOUT_MIN = 720;

static constexpr const char* SETUP_AP_SSID = "Zendure-Tempo-Setup";
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000;

static constexpr size_t MAX_ZENDURE_DEVICES = 8;

static constexpr uint32_t WEATHER_REFRESH_MS = 15UL * 60UL * 1000UL;
static constexpr const char* GEOCODING_HOST = "geocoding-api.open-meteo.com";
static constexpr const char* WEATHER_HOST = "api.open-meteo.com";
