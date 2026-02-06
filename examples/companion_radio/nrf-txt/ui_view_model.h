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

constexpr uint8_t kMessageSenderSize = 24;
constexpr uint8_t kMessageTextSize = 128;
constexpr uint8_t kMessageBufferSize = 128;

struct MessageEntry {
  bool read;
  uint32_t timestamp_ms;
  char sender[kMessageSenderSize];
  char message[kMessageTextSize];
};

using PromptCallback = void (*)(void* context, int result);
using TextInputCallback = void (*)(void* context, const char* text);

// Abstracts the hardware APIs from the UI.
class UIViewModel {
public:
  virtual uint32_t getMsgCount() = 0;
  virtual uint32_t getUnreadMsgCount() = 0;
  virtual bool isConnected() = 0;
  virtual bool isBuzzerEnabled() = 0;
  virtual void prompt(
    const char* title,
    const char* const* items,
    uint8_t count,
    PromptCallback callback,
    void* context) = 0;
  virtual void promptText(
    const char* title,
    char* buffer,
    uint8_t capacity,
    TextInputCallback callback,
    void* context) = 0;
  virtual bool sendChannelMessage(uint8_t channel_index, const char* text) = 0;
  virtual uint8_t getChannelSlots(uint8_t* slots, uint8_t max) = 0;
  virtual const char* getChannelName(uint8_t slot) = 0;
  virtual uint8_t getContactSlots(uint8_t* slots, uint8_t max) = 0;
  virtual const char* getContactName(uint8_t slot) = 0;
  virtual bool sendContactMessage(uint8_t slot, const char* text) = 0;
  virtual uint32_t getBlePin() = 0;
  virtual uint32_t getUptimeMin() = 0;
  virtual bool isBleEnabled() = 0;
  virtual void toggleBle() = 0;
  virtual void gotoHome() = 0;
  virtual void gotoMsgViewer(uint8_t offset) = 0;
  virtual void renderAfter(uint32_t delay_ms) = 0;
  virtual void shutdown(bool restart) = 0;
  virtual void toggleBuzzer() = 0;
  virtual bool sendAdvert() = 0;
  virtual void setGpsEnabled(bool enabled) = 0;
  virtual Position getPosition() = 0;
  virtual DateTime2 getDateTime() = 0;
  virtual RadioDetails getRadioDetails() = 0;
  virtual void resetRadioStats() = 0;
  // offset 0 is the oldest message.
  virtual uint8_t getMessages(uint8_t offset, uint8_t count, MessageEntry* out) = 0;
  virtual void markMessageRead(uint8_t offset) = 0;
  virtual const char* getFirmwareVersion() = 0;
  virtual float getBatteryPercent() = 0;
};
