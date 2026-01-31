#pragma once
#include <cstdint>

struct Position {
  float latitude;
  float longitude;
  float elevation;
  uint8_t satellites;
  bool has_fix;
  bool enabled;
};

struct DateTime2 {
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

// Abstracts the hardware APIs from the UI.
class UIViewModel {
public:
  virtual uint32_t getBlePin() = 0;
  virtual uint32_t getUptimeMin() = 0;
  virtual void gotoHome() = 0;
  virtual void renderAfter(uint32_t delay_ms) = 0;
  virtual void shutdown(bool restart) = 0;
  virtual void toggleBuzzer() = 0;
  virtual void toggleGPS() = 0;
  virtual Position getPosition() = 0;
  virtual DateTime2 getDateTime() = 0;
};