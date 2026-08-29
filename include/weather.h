#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

void pollWeather();
bool searchWeatherCity(const String& query, JsonDocument& out);
