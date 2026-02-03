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

struct RadioDetails {
  float frequency;
  uint8_t spreading_factor;
  float bandwidth;
  uint8_t coding_factor;
  uint8_t transmit_power_dbm;
  int16_t noise_floor_dbm;
  float last_rssi_dbm;
  float last_snr_db;
  uint32_t packets_sent;
  uint32_t packets_received;
};

// Abstracts the hardware APIs from the UI.
class UIViewModel {
public:
  virtual uint32_t getMsgCount() = 0;
  virtual bool isConnected() = 0;
  virtual bool isBuzzerEnabled() = 0;
  virtual uint32_t getBlePin() = 0;
  virtual uint32_t getUptimeMin() = 0;
  virtual void gotoHome() = 0;
  virtual void renderAfter(uint32_t delay_ms) = 0;
  virtual void shutdown(bool restart) = 0;
  virtual void toggleBuzzer() = 0;
  virtual void toggleGPS() = 0;
  virtual Position getPosition() = 0;
  virtual DateTime2 getDateTime() = 0;
  virtual RadioDetails getRadioDetails() = 0;
  virtual void resetRadioStats() = 0;
  virtual const char* getFirmwareVersion() = 0;
  virtual float getBatteryPercent() = 0;
};
