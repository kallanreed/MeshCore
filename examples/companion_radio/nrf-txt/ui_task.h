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
  uint32_t _next_batt_chck = 0;
  uint32_t _auto_off = 0;
  uint32_t _alert_expiry = 0;
  uint32_t _next_backlight_btn_check = 0;

  uint8_t _alert[80] = {};
  uint32_t _msgcount = 0;

  UIScreen* _splash;
  UIScreen* _home;
  UIScreen* _msg_preview;
  UIScreen* _quick_msg;
  UIScreen* _curr;

  void userLedHandler();

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
  uint32_t getBlePin() override;
  uint32_t getUptimeMin() ;
  void gotoHome() override { setCurrent(_home); }
  void renderAfter(uint32_t delay_ms) override;
  void shutdown(bool restart = false) override;
  void toggleBuzzer() override;
  void toggleGPS() override;
  
  // void showAlert(const char* text, int duration_millis);
  // void gotoHomeScreen() { setCurrScreen(home); }
  // void gotoQuickMsgScreen() { setCurrScreen(quick_msg); }
  // int  getMsgCount() const { return _msgcount; }
  // bool hasDisplay() const { return _display != NULL; }
  // bool isButtonPressed() const;

  // bool getGPSState();
};
