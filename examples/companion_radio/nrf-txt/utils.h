#pragma once

#include <Arduino.h>
#include <stdio.h>
#include "sensor.h"

class Utils {
public:
  template <typename T, size_t N>
  static constexpr size_t countof(const T (&)[N]) {
    return N;
  }
  static float toF(float c) {
    return (c * 9.0f / 5.0f) + 32.0f;
  }

  static float toInHg(float hpa) {
    return hpa * 0.0295299831f;
  }

  static void formatAgeSeconds(char* out, size_t size, uint32_t age_sec) {
    if (!out || size == 0)
      return;
    if (age_sec < 60) {
      snprintf(out, size, "%lus", static_cast<unsigned long>(age_sec));
    } else if (age_sec < 60 * 60) {
      snprintf(out, size, "%lum", static_cast<unsigned long>(age_sec / 60));
    } else if (age_sec < 60 * 60 * 24) {
      snprintf(out, size, "%luh", static_cast<unsigned long>(age_sec / (60 * 60)));
    } else {
      snprintf(out, size, "%lud", static_cast<unsigned long>(age_sec / (60 * 60 * 24)));
    }
  }

  static void formatAgeMillis(char* out, size_t size, uint32_t timestamp_ms) {
    uint32_t age_sec = (millis() - timestamp_ms) / 1000;
    formatAgeSeconds(out, size, age_sec);
  }

  static uint8_t safeStrLen(const char* text, uint8_t max_len) {
    if (!text)
      return 0;

    for (uint8_t i = 0; i < max_len; i++) {
      if (text[i] == 0)
        return i;
    }

    return max_len;
  }

  static const char* metricTitle(Bme680Metric metric) {
    switch (metric) {
      case Bme680Metric::temperature: return "Temp (F)";
      case Bme680Metric::humidity: return "Humidity (%)";
      case Bme680Metric::pressure: return "Pressure (inHg)";
      case Bme680Metric::gas: return "Gas (ohm)";
      default: break;
    }
    return "";
  }

  static float convertMetric(Bme680Metric metric, float value) {
    switch (metric) {
      case Bme680Metric::temperature: return toF(value);
      case Bme680Metric::pressure: return toInHg(value);
      default: break;
    }
    return value;
  }

  static void formatMetricValue(char* out, size_t out_size, Bme680Metric metric, float value) {
    if (!out || out_size == 0)
      return;

    switch (metric) {
      case Bme680Metric::temperature:
        snprintf(out, out_size, "%.0f", value);
        break;
      case Bme680Metric::humidity:
        snprintf(out, out_size, "%.1f", value);
        break;
      case Bme680Metric::pressure:
        snprintf(out, out_size, "%.2f", value);
        break;
      case Bme680Metric::gas:
        snprintf(out, out_size, "%.0f", value);
        break;
      default:
        snprintf(out, out_size, "%.2f", value);
        break;
    }
  }

  static void formatTemp(char* out, size_t out_size, float c) {
    if (!out || out_size == 0)
      return;
    snprintf(out, out_size, "%.0fF", toF(c));
  }

  static void formatHumidity(char* out, size_t out_size, float h) {
    if (!out || out_size == 0)
      return;
    snprintf(out, out_size, "%.1f%%", h);
  }

  static void formatPressure(char* out, size_t out_size, float hpa) {
    if (!out || out_size == 0)
      return;
    snprintf(out, out_size, "%.2finHg", toInHg(hpa));
  }

  static void formatGas(char* out, size_t out_size, float ohms) {
    if (!out || out_size == 0)
      return;
    if (ohms <= 0.0f) {
      snprintf(out, out_size, "n/a");
      return;
    }
    if (ohms >= 1000.0f) {
      snprintf(out, out_size, "%.1fkohm", ohms / 1000.0f);
    } else {
      snprintf(out, out_size, "%.0fohm", ohms);
    }
  }
};
