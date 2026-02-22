#pragma once

#include <stdint.h>

struct Bme680Data {
  bool available;
  bool has_temperature;
  bool has_humidity;
  bool has_pressure;
  bool has_gas;
  float temperature;
  float humidity;
  float pressure;
  float gas_resistance;
};

enum class Bme680Metric : uint8_t {
  temperature,
  humidity,
  pressure,
  gas
};

class Bme680HistoryStore {
public:
  static constexpr uint8_t kHistorySize = 60;
  static constexpr uint32_t kHistoryIntervalMs = 60000;

  void clear();
  void tick(uint32_t now_ms, const Bme680Data& data);
  uint8_t get(Bme680Metric metric, float* out, uint8_t max) const;

private:
  struct History {
    float values[kHistorySize] = {};
    uint8_t count = 0;
    uint8_t head = 0;
  };

  uint32_t _next_sample = 0;
  History _temp;
  History _humidity;
  History _pressure;
  History _gas;

  void push(History& history, float value);
  uint8_t copy(const History& history, float* out, uint8_t max) const;
};

bool decodeBme680FromLpp(const uint8_t* buf, uint8_t len, Bme680Data& out);
