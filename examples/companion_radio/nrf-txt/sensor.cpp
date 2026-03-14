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
  _interval_multiplier = 1;
  _latest = {};
  _temp = {};
  _humidity = {};
  _pressure = {};
}

void Bme680HistoryStore::compactHistory(History& history) {
  if (history.count < 2)
    return;

  uint8_t out = 0;
  for (uint8_t i = 0; i < history.count; i += 2) {
    history.values[out++] = history.values[i];
  }
  history.count = out;
}

void Bme680HistoryStore::compact() {
  compactHistory(_temp);
  compactHistory(_humidity);
  compactHistory(_pressure);
  if (_interval_multiplier < kMaxIntervalMultiplier)
    _interval_multiplier <<= 1;
}

void Bme680HistoryStore::push(History& history, float value) {
  if (history.count >= kHistorySize) {
    memmove(history.values, history.values + 1, sizeof(float) * (kHistorySize - 1));
    history.count = kHistorySize - 1;
  }

  history.values[history.count++] = value;
}

uint8_t Bme680HistoryStore::copy(const History& history, float* out, uint8_t max) const {
  if (!out || max == 0 || history.count == 0)
    return 0;

  uint8_t count = history.count < max ? history.count : max;
  memcpy(out, history.values, sizeof(float) * count);
  return count;
}

void Bme680HistoryStore::tick(uint32_t now_ms, const Bme680Data& data) {
  if (now_ms < _next_sample)
    return;

  _latest = data;
  if (!data.available)
  {
    _next_sample = now_ms + (kBaseHistoryIntervalMs * _interval_multiplier);
    return;
  }

  bool should_compact = false;
  if (data.has_temperature && _temp.count >= kHistorySize)
    should_compact = true;
  if (data.has_humidity && _humidity.count >= kHistorySize)
    should_compact = true;
  if (data.has_pressure && _pressure.count >= kHistorySize)
    should_compact = true;

  if (should_compact && _interval_multiplier < kMaxIntervalMultiplier)
    compact();

  if (data.has_temperature)
    push(_temp, data.temperature);
  if (data.has_humidity)
    push(_humidity, data.humidity);
  if (data.has_pressure)
    push(_pressure, data.pressure);

  _next_sample = now_ms + (kBaseHistoryIntervalMs * _interval_multiplier);
}

uint8_t Bme680HistoryStore::get(Bme680Metric metric, float* out, uint8_t max) const {
  switch (metric) {
    case Bme680Metric::temperature:
      return copy(_temp, out, max);
    case Bme680Metric::humidity:
      return copy(_humidity, out, max);
    case Bme680Metric::pressure:
      return copy(_pressure, out, max);
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
      pos += 2;
      continue;
    }

    uint8_t skip = lppValueSize(type);
    if (pos + skip > len)
      break;
    pos += skip;
  }

  return out.available;
}
