#pragma once

#include <array>
#include <helpers/ui/UIScreen.h>
#include "controls.h"
#include "ui_view_model.h"

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
  bool _has_message = false;
  MessageEntry _message = {};
  MessageScope _scope = MessageScope::all;

  void setMessageInternal(const MessageEntry& message);

public:
  MsgViewer(UIViewModel* model);
  void setMessage(const MessageEntry& message, MessageScope scope);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};

class ThreadScreen : public UIScreen
{
  UIViewModel* _model;
  MessageList _list = MessageList(0, 24, 240, 112, 20);
  bool _is_contact = true;
  uint8_t _target = 0;
  char _title[48] = {};
  char _text[kMessageTextSize] = {};

  void refresh();
  static void onThreadText(void* context, const char* text);

public:
  ThreadScreen(UIViewModel* model);
  void setContact(uint8_t contact_index);
  void setChannel(uint8_t channel_index);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void activate() override;
};

class TextInputScreen : public UIScreen
{
  UIViewModel* _model;
  char _title[48] = {};
  char* _buffer = nullptr;
  uint8_t _capacity = 0;
  uint8_t _length = 0;
  uint8_t _cursor = 0;
  TextInputCallback _callback = nullptr;
  void* _context = nullptr;

public:
  TextInputScreen(UIViewModel* model);
  void begin(
    const char* title,
    char* buffer,
    uint8_t capacity,
    TextInputCallback callback,
    void* context);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};

class HomeScreen : public UIScreen
{
  UIViewModel* _model;
  uint8_t _page = 0;
  std::array<UIPage*, 9> _pages;
  BatteryIndicator _batt = BatteryIndicator(205, 118);

  UIPage* current() { return _pages[_page]; }

public:
  HomeScreen(UIViewModel* model);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
  void activate() override;
  void poll() override;
};
