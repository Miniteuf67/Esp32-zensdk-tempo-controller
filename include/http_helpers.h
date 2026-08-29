#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

bool httpGetJson(const String& url, JsonDocument& doc, int timeoutMs = 3500);
