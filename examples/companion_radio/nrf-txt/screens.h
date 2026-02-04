#pragma once

#include <array>
#include <helpers/ui/UIScreen.h>
#include "controls.h"
#include "ui_view_model.h"

/*
  UIScreen API
  int render(DisplayDriver& display) override;
  virtual bool handleInput(char c) { return false; }
  virtual void poll() { }
*/

class SplashScreen : public UIScreen
{
  UIViewModel* _model;
  uint32_t dismiss_after;
  char _version_info[12];

public:
  SplashScreen(UIViewModel* _model);
  int render(DisplayDriver& display) override;
  void poll() override;
};

class MsgViewer : public UIScreen
{
  UIViewModel* _model;
  uint8_t _offset = 0;
  bool _has_message = false;
  MessageEntry _message = {};

  void loadMessage();

public:
  MsgViewer(UIViewModel* model);
  void setOffset(uint8_t offset);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};

class HomeScreen : public UIScreen
{
  UIViewModel* _model;
  uint8_t _page = 0;
  std::array<UIPage*, 8> _pages;
  BatteryIndicator _batt = BatteryIndicator(205, 118);

  UIPage* current() { return _pages[_page]; }

public:
  HomeScreen(UIViewModel* model);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void poll() override;
};
