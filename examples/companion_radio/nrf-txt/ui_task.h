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
  genericBuzzer _buzzer;
  CardKB _keyboard = CardKB(&vext_power);

  uint32_t _ui_started_at = 0;
  uint32_t _next_render = 0;
  uint32_t _next_batt_check = 0;
  uint32_t _auto_off = 0;
  //uint32_t _alert_expiry = 0;
  //uint32_t _next_backlight_btn_check = 0;

  float _batt_percent = 0;

  uint8_t _alert[80] = {};
  uint32_t _msgcount = 0;
  MessageBuffer _message_buffer;

  UIScreen* _splash;
  UIScreen* _home;
  UIScreen* _msg_viewer;
  UIScreen* _text_input;
  UIScreen* _curr;
  MenuPrompt _prompt;

  void dispatchRender();
  void setCurrent(UIScreen* screen);
  bool wakeScreen();
  void checkAutoOff();

public:
  UITask(mesh::MainBoard* board, BaseSerialInterface* serial)
    : AbstractUITask(board, serial)
  { }

  void begin(
    DisplayDriver* display,
    SensorManager* sensors,
    NodePrefs* node_prefs);

  // AbstractUITask impl
  void msgRead(int msgcount) override;
  void newMsg(
    uint8_t path_len,
    const char* from_name,
    const char* text,
    int msgcount) override;
  void notify(UIEventType t = UIEventType::none) override;
  void loop() override;

  // UIViewModel impl
  uint32_t getMsgCount() override;
  bool isConnected() override;
  bool isBuzzerEnabled() override;
  void prompt(
    const char* title,
    const char* const* items,
    uint8_t count,
    PromptCallback callback,
    void* context) override;
  void promptText(
    char* buffer,
    uint8_t capacity,
    TextInputCallback callback,
    void* context) override;
  bool sendChannelMessage(uint8_t channel_index, const char* text) override;
  uint32_t getBlePin() override;
  uint32_t getUptimeMin() ;
  void gotoHome() override { setCurrent(_home); }
  void gotoMsgViewer(uint8_t offset) override;
  void renderAfter(uint32_t delay_ms) override;
  void shutdown(bool restart = false) override;
  void toggleBuzzer() override;
  void setGpsEnabled(bool enabled) override;
  Position getPosition() override;
  DateTime2 getDateTime() override;
  RadioDetails getRadioDetails() override;
  void resetRadioStats() override;
  uint8_t getMessages(uint8_t offset, uint8_t count, MessageEntry* out) override;
  void markMessageRead(uint8_t offset) override;
  const char* getFirmwareVersion() override;
  float getBatteryPercent() override;
};
