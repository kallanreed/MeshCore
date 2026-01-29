#pragma once
#include <cstdint>

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
};