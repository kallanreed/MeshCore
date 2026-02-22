#include "sensor.h"

#include <string.h>
#include <helpers/sensors/LPPDataHelpers.h>

namespace {
uint8_t lppValueSize(uint8_t type) {
  switch (type) {
    case LPP_GPS: return 9;
    case LPP_POLYLINE: return 8;
    case LPP_GYROMETER:
    case LPP_ACCELEROMETER: return 6;
    case LPP_GENERIC_SENSOR:
    case LPP_FREQUENCY:
    case LPP_DISTANCE:
    case LPP_ENERGY:
    case LPP_UNIXTIME: return 4;
    case LPP_COLOUR: return 3;
    case LPP_ANALOG_INPUT:
    case LPP_ANALOG_OUTPUT:
    case LPP_LUMINOSITY:
    case LPP_TEMPERATURE:
    case LPP_CONCENTRATION:
    case LPP_BAROMETRIC_PRESSURE:
    case LPP_ALTITUDE:
    case LPP_VOLTAGE:
    case LPP_CURRENT:
    case LPP_DIRECTION:
    case LPP_POWER: return 2;
    default: return 1;
  }
}
}  // namespace

void Bme680HistoryStore::clear() {
  _next_sample = 0;
  _latest = {};
  _temp = {};
  _humidity = {};
  _pressure = {};
  _gas = {};
}

void Bme680HistoryStore::push(History& history, float value) {
  history.values[history.head] = value;
  history.head = (history.head + 1) % kHistorySize;
  if (history.count < kHistorySize)
    history.count++;
}

uint8_t Bme680HistoryStore::copy(const History& history, float* out, uint8_t max) const {
  if (!out || max == 0 || history.count == 0)
    return 0;

  uint8_t count = history.count < max ? history.count : max;
  uint8_t start = (history.count == kHistorySize) ? history.head : 0;
  for (uint8_t i = 0; i < count; i++) {
    uint8_t idx = (start + i) % kHistorySize;
    out[i] = history.values[idx];
  }
  return count;
}

void Bme680HistoryStore::tick(uint32_t now_ms, const Bme680Data& data) {
  if (now_ms < _next_sample)
    return;

  _next_sample = now_ms + kHistoryIntervalMs;
  _latest = data;
  if (!data.available)
    return;

  if (data.has_temperature)
    push(_temp, data.temperature);
  if (data.has_humidity)
    push(_humidity, data.humidity);
  if (data.has_pressure)
    push(_pressure, data.pressure);
  if (data.has_gas && data.gas_resistance > 0.0f)
    push(_gas, data.gas_resistance);
}

uint8_t Bme680HistoryStore::get(Bme680Metric metric, float* out, uint8_t max) const {
  switch (metric) {
    case Bme680Metric::temperature:
      return copy(_temp, out, max);
    case Bme680Metric::humidity:
      return copy(_humidity, out, max);
    case Bme680Metric::pressure:
      return copy(_pressure, out, max);
    case Bme680Metric::gas:
      return copy(_gas, out, max);
    default:
      break;
  }
  return 0;
}

bool decodeBme680FromLpp(const uint8_t* buf, uint8_t len, Bme680Data& out) {
  out = {};
  if (!buf || len < 2)
    return false;

  uint8_t pos = 0;
  while (pos + 2 <= len) {
    uint8_t channel = buf[pos++];
    uint8_t type = buf[pos++];
    if (channel == 0)
      break;

    if (type == LPP_TEMPERATURE) {
      if (pos + 2 > len)
        break;
      int16_t raw = static_cast<int16_t>((buf[pos] << 8) | buf[pos + 1]);
      float value = raw / static_cast<float>(LPP_TEMPERATURE_MULT);
      pos += 2;
      if (!out.has_temperature) {
        out.temperature = value;
        out.has_temperature = true;
      }
      out.available = true;
      continue;
    }

    if (type == LPP_RELATIVE_HUMIDITY) {
      if (pos + 1 > len)
        break;
      float value = buf[pos++] / static_cast<float>(LPP_RELATIVE_HUMIDITY_MULT);
      if (!out.has_humidity) {
        out.humidity = value;
        out.has_humidity = true;
      }
      out.available = true;
      continue;
    }

    if (type == LPP_BAROMETRIC_PRESSURE) {
      if (pos + 2 > len)
        break;
      uint16_t raw = (buf[pos] << 8) | buf[pos + 1];
      float value = raw / static_cast<float>(LPP_BAROMETRIC_PRESSURE_MULT);
      pos += 2;
      if (!out.has_pressure) {
        out.pressure = value;
        out.has_pressure = true;
      }
      out.available = true;
      continue;
    }

    if (type == LPP_ANALOG_INPUT) {
      if (pos + 2 > len)
        break;
      int16_t raw = static_cast<int16_t>((buf[pos] << 8) | buf[pos + 1]);
      float value = raw / static_cast<float>(LPP_ANALOG_INPUT_MULT);
      pos += 2;
      if (!out.has_gas && value > 0.0f) {
        out.gas_resistance = value;
        out.has_gas = true;
      }
      out.available = true;
      continue;
    }

    uint8_t skip = lppValueSize(type);
    if (pos + skip > len)
      break;
    pos += skip;
  }

  return out.available;
}
