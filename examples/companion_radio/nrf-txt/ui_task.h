#pragma once

#include <Arduino.h>
#include <MeshCore.h>
#include <cstdint>
#include <helpers/BaseSerialInterface.h>
#include <helpers/RefCountedDigitalPin.h>
#include <helpers/SensorManager.h>
#include <helpers/sensors/LPPDataHelpers.h>
#include <helpers/ui/buzzer.h>
#include <helpers/ui/DisplayDriver.h>
#include <helpers/ui/UIScreen.h>
#include <CayenneLPP.h>
#include "sensor.h"
#include "../AbstractUITask.h"
#include "../NodePrefs.h"
#include "nrf_hardware.h"
#include "ui_view_model.h"
#include "controls.h"

// Used to control whether Vext is powered.
extern RefCountedDigitalPin vext_power;

class UITask : public AbstractUITask, public UIViewModel {
  DisplayDriver* _display = nullptr;
  SensorManager* _sensors = nullptr;
  NodePrefs* _node_prefs = nullptr;
  CayenneLPP _sensors_lpp;
  genericBuzzer _buzzer;
  CardKB _keyboard = CardKB(&vext_power);

  uint32_t _ui_started_at = 0;
  uint32_t _next_render = 0;
  uint32_t _next_batt_check = 0;
  uint32_t _auto_off = 0;
  uint32_t _dm_ack_expires_at = 0;
  uint32_t _dm_ack_trip_time_ms = 0;
  //uint32_t _alert_expiry = 0;
  //uint32_t _next_backlight_btn_check = 0;

  float _batt_percent = 0;
  char _dm_ack_label[24] = {};
  MessageBuffer _message_buffer;
  bool _invert_screen = false;
  Bme680HistoryStore _bme680_history;

  UIScreen* _splash;
  UIScreen* _home;
  UIScreen* _msg_viewer;
  UIScreen* _thread_viewer;
  UIScreen* _text_input;
  UIScreen* _sensor_chart;
  UIScreen* _curr;
  UIScreen* _prev_screen;
  MenuPrompt _prompt;

  void dispatchRender();
  void setCurrent(UIScreen* screen);
  bool wakeScreen();
  void checkAutoOff();
  void updateBme680History();
  Bme680Data readBme680Data();
  void renderDmAckBadge();

public:
  UITask(mesh::MainBoard* board, BaseSerialInterface* serial)
    : AbstractUITask(board, serial)
    , _sensors_lpp(128)
  { }

  void begin(
    DisplayDriver* display,
    SensorManager* sensors,
    NodePrefs* node_prefs);

  // AbstractUITask impl
  void msgRead(int msgcount) override { }
  void newMsg(
    uint8_t path_len,
    const char* from_name,
    const char* text,
    int msgcount,
    const UIMessageMeta& meta) override;
  void onDirectMessageAck(const ContactInfo& contact, uint32_t trip_time_ms) override;
  void notify(UIEventType t = UIEventType::none) override;
  void loop() override;

  // UIViewModel impl
  uint32_t getMsgCount() override;
  uint32_t getUnreadMsgCount() override;
  void markAllMessagesRead() override;
  bool isConnected() override;
  bool isBuzzerEnabled() override;
  void prompt(
    const char* title,
    const char* const* items,
    uint8_t count,
    PromptCallback callback,
    void* context) override;
  void promptText(
    const char* title,
    char* buffer,
    uint8_t capacity,
    TextInputCallback callback,
    void* context) override;
  bool sendChannelMessage(uint8_t channel_index, const char* text) override;
  uint8_t getChannelIndexes(uint8_t* indexes, uint8_t max) override;
  const char* getChannelName(uint8_t channel_index) override;
  uint8_t getMsgCountForChannel(uint8_t channel_index) override;
  uint8_t getUnreadCountForChannel(uint8_t channel_index) override;
  uint8_t getMessagesForChannel(
    uint8_t channel_index,
    uint8_t offset,
    uint8_t count,
    MessageEntry* out) override;
  void markMessagesReadForChannel(uint8_t channel_index) override;
  void gotoChannelThread(uint8_t channel_index) override;
  uint8_t getContactIndexes(uint8_t* indexes, uint8_t max) override;
  const char* getContactName(uint8_t contact_index) override;
  bool sendContactMessage(uint8_t contact_index, const char* text) override;
  uint8_t getMsgCountForContact(uint8_t contact_index) override;
  uint8_t getUnreadCountForContact(uint8_t contact_index) override;
  uint8_t getMessagesForContact(
    uint8_t contact_index,
    uint8_t offset,
    uint8_t count,
    MessageEntry* out) override;
  void markMessagesReadForContact(uint8_t contact_index) override;
  void gotoContactThread(uint8_t contact_index) override;
  uint32_t getBlePin() override;
  uint32_t getUptimeMin() ;
  bool isBleEnabled() override;
  void toggleBle() override;
  bool isCampModeEnabled() override;
  void toggleCampMode() override;
  void gotoHome() override;
  void gotoPrevious() override;
  void gotoMsgViewer(const MessageEntry& message, MessageScope scope) override;
  void renderAfter(uint32_t delay_ms) override;
  void shutdown(bool restart = false) override;
  void toggleBuzzer() override;
  void toggleScreenInvert() override;
  bool sendAdvert(bool flood=false) override;
  void setGpsEnabled(bool enabled) override;
  void setTzOffset(int8_t offset_hours) override;
  Position getPosition() override;
  DateTime2 getDateTime() override;
  RadioDetails getRadioDetails() override;
  Bme680Data getBme680Data() override;
  uint8_t getBme680History(Bme680Metric metric, float* out, uint8_t max) override;
  uint8_t getBme680HistoryIntervalMin() override;
  void gotoChart(ChartConfig* config) override;
  void resetRadioStats() override;
  uint8_t getMessages(uint8_t offset, uint8_t count, MessageEntry* out) override;
  void markMessageRead(uint8_t offset) override;
  void markMessageReadById(uint32_t message_id) override;
  bool getPreviousMessage(MessageScope scope, const MessageEntry& current, MessageEntry* out) override;
  bool getNextMessage(MessageScope scope, const MessageEntry& current, MessageEntry* out) override;
  uint8_t getRecentAdverts(RecentAdvertEntry* out, uint8_t max) override;
  bool hasContact(const uint8_t* pub_key) override;
  bool addRecentAdvertContact(const RecentAdvertEntry& advert) override;
  uint32_t getRtcSeconds() override;
  const char* getFirmwareVersion() override;
  float getBatteryPercent() override;
  const char* getNodeName() override;
};
