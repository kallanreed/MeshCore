#pragma once

#include <stdint.h>

struct Bme680Data {
  bool available;
  bool has_temperature;
  bool has_humidity;
  bool has_pressure;
  float temperature;
  float humidity;
  float pressure;
};

enum class Bme680Metric : uint8_t {
  temperature,
  humidity,
  pressure
};

class Bme680HistoryStore {
public:
  static constexpr uint8_t kHistorySize = 64;
  static constexpr uint32_t kBaseHistoryIntervalMs = 60 * 1000;
  static constexpr uint8_t kMaxIntervalMultiplier = 16;

  void clear();
  void tick(uint32_t now_ms, const Bme680Data& data);
  uint8_t get(Bme680Metric metric, float* out, uint8_t max) const;
  bool needsSample(uint32_t now_ms) const { return now_ms >= _next_sample; }
  Bme680Data latest() const { return _latest; }
  uint8_t getIntervalMinutes() const { return _interval_multiplier; }

private:
  struct History {
    float values[kHistorySize] = {};
    uint8_t count = 0;
  };

  uint32_t _next_sample = 0;
  uint8_t _interval_multiplier = 1;
  Bme680Data _latest = {};
  History _temp;
  History _humidity;
  History _pressure;

  void compact();
  void compactHistory(History& history);
  void push(History& history, float value);
  uint8_t copy(const History& history, float* out, uint8_t max) const;
};

bool decodeBme680FromLpp(const uint8_t* buf, uint8_t len, Bme680Data& out);
