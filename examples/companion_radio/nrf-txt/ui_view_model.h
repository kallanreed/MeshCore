#pragma once
#include <cstdint>
#include "sensor.h"

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
  int8_t tz_offset;
  bool is_valid;
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
constexpr uint8_t kContactPrefixSize = 6; // First bytes of public key for UI association.
constexpr uint8_t kMessageTextSize = 141; // Max bytes stored per message text (including null terminator).
constexpr uint8_t kMessageBufferSize = 128; // Ring buffer capacity in number of messages.
constexpr uint8_t kRecentAdvertNameSize = 32;
constexpr uint8_t kRecentAdvertMax = 8;
constexpr uint8_t kRecentAdvertKeySize = 32; // PUB_KEY_SIZE

enum class MessageKind : uint8_t {
  unknown,
  contact,
  channel
};

enum class MessageDirection : uint8_t {
  incoming,
  outgoing
};

enum class MessageScope : uint8_t {
  all,
  thread
};

struct MessageEntry {
  uint32_t timestamp_ms;
  char sender[kMessageSenderSize];
  char message[kMessageTextSize];
  uint8_t channel_index;
  uint8_t contact_prefix[kContactPrefixSize];
  uint8_t flags;

  bool isRead() const { return (flags & 0x01) != 0; }
  void setRead(bool value) { flags = value ? (flags | 0x01) : (flags & ~0x01); }

  bool isAcked() const { return (flags & 0x02) != 0; }
  void setAcked(bool value) { flags = value ? (flags | 0x02) : (flags & ~0x02); }

  MessageDirection direction() const {
    return (flags & 0x04) ? MessageDirection::outgoing : MessageDirection::incoming;
  }
  void setDirection(MessageDirection value) {
    flags = (value == MessageDirection::outgoing) ? (flags | 0x04) : (flags & ~0x04);
  }

  MessageKind kind() const {
    return static_cast<MessageKind>((flags >> 3) & 0x03);
  }
  void setKind(MessageKind value) {
    flags = static_cast<uint8_t>((flags & ~0x18) | ((static_cast<uint8_t>(value) & 0x03) << 3));
  }

  uint8_t heardRepeatState() const {
    return (flags >> 5) & 0x03;
  }
  void setHeardRepeatState(uint8_t value) {
    flags = static_cast<uint8_t>((flags & ~0x60) | ((value & 0x03) << 5));
  }
  void advanceHeardRepeatState() {
    uint8_t state = heardRepeatState();
    if (state < 3)
      setHeardRepeatState(state + 1);
  }
};

struct RecentAdvertEntry {
  char name[kRecentAdvertNameSize];
  uint32_t recv_timestamp;
  uint8_t pub_key[kRecentAdvertKeySize];
};

using PromptCallback = void (*)(void* context, int result);
using TextInputCallback = void (*)(void* context, const char* text);
using ChartFetchCallback = uint8_t (*)(void* context, float* out, uint8_t max);
using ChartFormatCallback = void (*)(void* context, char* out, uint8_t out_size, float value);
using ChartStatusCallback = void (*)(void* context, char* out, uint8_t out_size);

struct ChartConfig {
  const char* title;
  uint8_t expected_samples;
  ChartFetchCallback fetch;
  ChartFormatCallback format;
  ChartStatusCallback status;
  void* context;
};

// Abstracts the hardware APIs from the UI.
class UIViewModel {
public:
  virtual uint32_t getMsgCount() = 0;
  virtual uint32_t getUnreadMsgCount() = 0;
  virtual void markAllMessagesRead() = 0;
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
  virtual uint8_t getChannelIndexes(uint8_t* indexes, uint8_t max) = 0;
  virtual const char* getChannelName(uint8_t channel_index) = 0;
  virtual uint8_t getMsgCountForChannel(uint8_t channel_index) = 0;
  virtual uint8_t getUnreadCountForChannel(uint8_t channel_index) = 0;
  virtual uint8_t getMessagesForChannel(
    uint8_t channel_index,
    uint8_t offset,
    uint8_t count,
    MessageEntry* out) = 0;
  virtual void markMessagesReadForChannel(uint8_t channel_index) = 0;
  virtual bool deleteChannel(uint8_t channel_index) = 0;
  virtual void gotoChannelThread(uint8_t channel_index) = 0;
  virtual uint8_t getContactIndexes(uint8_t* indexes, uint8_t max) = 0;
  virtual const char* getContactName(uint8_t contact_index) = 0;
  virtual bool sendContactMessage(uint8_t contact_index, const char* text) = 0;
  virtual uint8_t getMsgCountForContact(uint8_t contact_index) = 0;
  virtual uint8_t getUnreadCountForContact(uint8_t contact_index) = 0;
  virtual uint8_t getMessagesForContact(
    uint8_t contact_index,
    uint8_t offset,
    uint8_t count,
    MessageEntry* out) = 0;
  virtual void markMessagesReadForContact(uint8_t contact_index) = 0;
  virtual bool deleteContact(uint8_t contact_index) = 0;
  virtual void gotoContactThread(uint8_t contact_index) = 0;
  virtual uint32_t getBlePin() = 0;
  virtual uint32_t getUptimeMin() = 0;
  virtual bool isBleEnabled() = 0;
  virtual void toggleBle() = 0;
  virtual bool isCampModeEnabled() = 0;
  virtual void toggleCampMode() = 0;
  virtual void gotoHome() = 0;
  virtual void gotoPrevious() = 0;
  virtual void gotoMsgViewer(const MessageEntry& message, MessageScope scope) = 0;
  virtual bool openComposeForMessage(const MessageEntry& message) = 0;
  virtual void renderAfter(uint32_t delay_ms) = 0;
  virtual void shutdown(bool restart) = 0;
  virtual void toggleBuzzer() = 0;
  virtual void toggleScreenInvert() = 0;
  virtual bool sendAdvert(bool flood=false) = 0;
  virtual void setGpsEnabled(bool enabled) = 0;
  virtual void setTzOffset(int8_t offset_hours) = 0;
  virtual Position getPosition() = 0;
  virtual DateTime2 getDateTime() = 0;
  virtual RadioDetails getRadioDetails() = 0;
  virtual Bme680Data getBme680Data() = 0;
  virtual uint8_t getBme680History(Bme680Metric metric, float* out, uint8_t max) = 0;
  virtual uint8_t getBme680HistoryIntervalMin() = 0;
  virtual void gotoChart(ChartConfig* config) = 0;
  virtual void resetRadioStats() = 0;
  // offset 0 is the oldest message.
  virtual uint8_t getMessages(uint8_t offset, uint8_t count, MessageEntry* out) = 0;
  virtual bool getMessageByTimestamp(uint32_t timestamp_ms, MessageEntry* out) = 0;
  virtual void markMessageRead(uint8_t offset) = 0;
  virtual void markMessageReadByTimestamp(uint32_t timestamp_ms) = 0;
  virtual bool getPreviousMessage(MessageScope scope, const MessageEntry& current, MessageEntry* out) = 0;
  virtual bool getNextMessage(MessageScope scope, const MessageEntry& current, MessageEntry* out) = 0;
  virtual uint8_t getRecentAdverts(RecentAdvertEntry* out, uint8_t max) = 0;
  virtual bool hasContact(const uint8_t* pub_key) = 0;
  virtual bool addRecentAdvertContact(const RecentAdvertEntry& advert) = 0;
  virtual uint32_t getRtcSeconds() = 0;
  virtual const char* getFirmwareVersion() = 0;
  virtual float getBatteryPercent() = 0;
  virtual const char* getNodeName() = 0;
};
