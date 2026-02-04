#pragma once

#include <stdio.h>
#include <string.h>
#include <helpers/ui/UIScreen.h>
#include "icons.h"
#include "ui_view_model.h"

// A 7-segment display UI element.
class _7Seg {
private:
  uint8_t _x;
  uint8_t _y;
  uint8_t _value = 0;

  // | - |  6, 5, 4
  //   -    3
  // | - |  2, 1, 0
  uint8_t getBits() {
    switch (_value) {
      case 1:
        return 0b0010001;
      case 2:
        return 0b0111110;
      case 3:
        return 0b0111011;
      case 4:
        return 0b1011001;
      case 5:
        return 0b1101011;
      case 6:
        return 0b1101111;
      case 7:
        return 0b0110001;
      case 8:
        return 0b1111111;
      case 9:
        return 0b1111001;
      default:
        return 0b1110111;
    }
  }

public:
  _7Seg(uint8_t x, uint8_t y) : _x(x), _y(y) {}

  void set(uint8_t value) { _value = value; }

  void render(DisplayDriver& display) {
    auto bits = getBits();

    if (bits & 0x40) // TL
      display.fillRect(_x, _y + 3, 3, 19);

    if (bits & 0x20) // TC
      display.fillRect(_x + 3, _y, 19, 3);

    if (bits & 0x10) // TR
      display.fillRect(_x + 22, _y + 3, 3, 19);

    if (bits & 0x8) // CC
      display.fillRect(_x + 3, _y + 22, 19, 3);

    if (bits & 0x4) // BL
      display.fillRect(_x, _y + 25, 3, 19);

    if (bits & 0x2) // BC
      display.fillRect(_x + 3, _y + 44, 19, 3);

    if (bits & 0x1) // BR
      display.fillRect(_x + 22, _y + 25, 3, 19);
  }
};

class BatteryIndicator {
  uint8_t _x;
  uint8_t _y;

public:
  BatteryIndicator(uint8_t x, uint8_t y) : _x(x), _y(y) {}

  void render(DisplayDriver& display, float percent) {
    char tmp[5];
    sprintf(tmp, "%d", static_cast<uint8_t>(100 * percent));

    display.setColor(DisplayDriver::LIGHT);
    display.drawXbm(_x, _y, icon_batt, 16, 8, 2);

    display.setColor(display.INVERSE);
    display.fillRect(_x + 2, _y + 2, 24 * percent, 12);
    display.setTextSize(1);
    display.drawTextCentered(_x + 14, _y + 5, tmp);

    display.setColor(DisplayDriver::LIGHT);
  }
};

class MenuPrompt {
  const char* _title = nullptr;
  const char* const* _items = nullptr;
  uint8_t _count = 0;
  int8_t _selected = 0;
  bool _active = false;
  PromptCallback _callback = nullptr;
  void* _context = nullptr;

  void finish(int result) {
    _active = false;
    if (_callback)
      _callback(_context, result);
    _callback = nullptr;
    _context = nullptr;
  }

public:
  void begin(
    const char* title,
    const char* const* items,
    uint8_t count,
    PromptCallback callback,
    void* context) {
    _title = title;
    _items = items;
    _count = count;
    _selected = 0;
    _active = count > 0;
    _callback = callback;
    _context = context;
  }

  bool isActive() const { return _active; }

  void render(DisplayDriver& display) {
    if (!_active)
      return;

    auto box_w = 180;
    auto box_h = 135;
    auto left = (display.width() - box_w) / 2;
    auto top = 0;
    auto center_x = left + (box_w / 2);

    display.setColor(DisplayDriver::DARK);
    display.fillRect(left, top, box_w, box_h);
    display.setColor(DisplayDriver::LIGHT);
    display.drawRect(left + 2, top + 2, box_w - 4, box_h - 4);

    display.setTextSize(2);
    display.drawTextCentered(center_x, top + 4, _title ? _title : "");
    display.drawRect(left + 6, top + 24, box_w - 12, 1);

    int y = top + 30;
    int item_w = box_w - 12;
    display.setTextSize(2);
    for (uint8_t i = 0; i < _count; i++, y += 20) {
      display.drawTextLeftAlign(left + 10, y, _items[i] ? _items[i] : "");
      if (i == _selected) {
        display.setColor(DisplayDriver::INVERSE);
        display.fillRect(left + 6, y, item_w, 18);
        display.setColor(DisplayDriver::LIGHT);
      }
    }
  }

  void handleInput(char c) {
    if (!_active)
      return;

    switch (c) {
      case KEY_UP:
        if (_count > 0)
          _selected = (_selected + _count - 1) % _count;
        break;
      case KEY_DOWN:
        if (_count > 0)
          _selected = (_selected + 1) % _count;
        break;
      case KEY_ENTER:
        finish(_selected);
        break;
      case KEY_CANCEL:
        finish(-1);
        break;
      default:
        break;
    }
  }
};

using ScrollListItemRenderer = void (*)(DisplayDriver& display,
  uint8_t index,
  int x,
  int y,
  int w,
  int h,
  void* context);

class ScrollList {
  int _x = 0;
  int _y = 0;
  int _w = 0;
  int _h = 0;
  uint8_t _row_h = 0;
  uint8_t _count = 0;
  uint8_t _selected = 0;
  uint8_t _top = 0;
  ScrollListItemRenderer _renderer = nullptr;
  void* _context = nullptr;

  void clampTop() {
    auto visible = getVisibleCount();
    if (visible == 0 || _count == 0) {
      _top = 0;
      return;
    }

    if (_selected < _top)
      _top = _selected;
    else if (_selected >= _top + visible)
      _top = _selected - visible + 1;
  }

public:
  ScrollList(int x, int y, int w, int h, uint8_t row_h)
    : _x(x), _y(y), _w(w), _h(h), _row_h(row_h) {}

  void setRenderer(ScrollListItemRenderer renderer, void* context) {
    _renderer = renderer;
    _context = context;
  }

  void setCount(uint8_t count) {
    _count = count;
    if (_count == 0) {
      reset();
    } else if (_selected >= _count) {
      _selected = _count - 1;
      clampTop();
    }
  }

  uint8_t getCount() const { return _count; }
  uint8_t getSelected() const { return _selected; }
  uint8_t getTop() const { return _top; }

  void reset() {
    _selected = 0;
    _top = 0;
  }

  uint8_t getVisibleCount() const {
    return _row_h == 0 ? 0 : static_cast<uint8_t>(_h / _row_h);
  }

  bool handleInput(char c) {
    if (_count == 0)
      return false;

    switch (c) {
      case KEY_UP:
        _selected = (_selected + _count - 1) % _count;
        clampTop();
        return true;
      case KEY_DOWN:
        _selected = (_selected + 1) % _count;
        clampTop();
        return true;
      default:
        return false;
    }
  }

  void render(DisplayDriver& display) {
    if (!_renderer)
      return;

    auto visible = getVisibleCount();
    for (uint8_t row = 0; row < visible; row++) {
      uint8_t index = _top + row;
      if (index >= _count)
        break;

      int y = _y + (row * _row_h);

      display.setColor(DisplayDriver::LIGHT);
      _renderer(display, index, _x + 2, y + 2, _w - 4, _row_h - 4, _context);

      if (index == _selected) {
        display.setColor(DisplayDriver::INVERSE);
        display.fillRect(_x, y, _w, _row_h);
      }
    }
  }
};

class MessageBuffer {
  MessageEntry _entries[kMessageBufferSize] = {};
  uint8_t _head = 0;
  uint8_t _count = 0;

  uint8_t toIndex(uint8_t offset) const {
    return (_head + kMessageBufferSize - 1 - offset) % kMessageBufferSize;
  }

public:
  uint8_t getCount() const { return _count; }

  void addMessage(uint32_t timestamp_ms, const char* sender, const char* message) {
    auto* entry = &_entries[_head];
    entry->read = false;
    entry->timestamp_ms = timestamp_ms;

    if (sender) {
      strncpy(entry->sender, sender, sizeof(entry->sender));
      entry->sender[sizeof(entry->sender) - 1] = 0;
    } else {
      entry->sender[0] = 0;
    }

    if (message) {
      strncpy(entry->message, message, sizeof(entry->message));
      entry->message[sizeof(entry->message) - 1] = 0;
    } else {
      entry->message[0] = 0;
    }

    _head = (_head + 1) % kMessageBufferSize;
    if (_count < kMessageBufferSize)
      _count++;
  }

  uint8_t getMessages(uint8_t offset, uint8_t count, MessageEntry* out) const {
    if (!out || count == 0 || offset >= _count)
      return 0;

    uint8_t available = _count - offset;
    uint8_t to_copy = count < available ? count : available;

    for (uint8_t i = 0; i < to_copy; i++) {
      out[i] = _entries[toIndex(offset + i)];
    }

    return to_copy;
  }

  void markRead(uint8_t offset) {
    if (offset >= _count)
      return;

    _entries[toIndex(offset)].read = true;
  }

  void markAllRead() {
    for (uint8_t i = 0; i < _count; i++) {
      _entries[toIndex(i)].read = true;
    }
  }
};

// Interface type for pages hosted on the HomeScreen.
class UIPage {
protected:
  UIViewModel* _model;

public:
  UIPage(UIViewModel* model) : _model(model) {}

  // TODO: Title text?

  // An 8x8 XBM to show in the icon strip.
  virtual const uint8_t* getIcon() = 0;

  // This is what's displayed on the main screen when
  // the page is selected but not activated.
  // The icon strip will cover the bottom 20 pixels while shown.
  virtual void renderPreview(DisplayDriver& display) = 0;

  // Called when the page is shown.
  virtual void activate() {};

  // Returns true if the input is handled.
  virtual bool handleInput(char c) { return false; }
};

class HomePage : public UIPage {
  char _text[24];

public:
  HomePage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_home;
  }

  void renderPreview(DisplayDriver& display) override {
    auto center_x = display.width() / 2;
    display.setColor(DisplayDriver::LIGHT);

    if (_model->isBuzzerEnabled()) {
      display.drawXbm(222, 2, icon_snd_on, 8, 8, 2);
    }

    display.setTextSize(3);
    sprintf(_text, "MSG: %lu", _model->getMsgCount());
    display.drawTextCentered(center_x, 40, _text);

    display.setTextSize(2);
    if (_model->isConnected()) {
      display.drawTextCentered(center_x, 70, "Connected");
    } else {
      auto pin = _model->getBlePin();
      if (pin != 0) {
        sprintf(_text, "Pin: %lu", pin);
        display.drawTextCentered(center_x, 70, _text);
      }
    }
  }

  bool handleInput(char c) override {
    if (c != KEY_ENTER)
      return false;

    _model->toggleBuzzer();
    return true;
  }
};

class MsgPage : public UIPage {
  ScrollList _list = ScrollList(0, 0, 240, 116, 20);

  static void renderMessageItem(
    DisplayDriver& display,
    uint8_t index,
    int x,
    int y,
    int w,
    int h,
    void* context) {
    auto* page = static_cast<MsgPage*>(context);
    MessageEntry entry{};
    if (page->_model->getMessages(index, 1, &entry) == 0)
      return;

    char sender[kMessageSenderSize];
    char message[kMessageTextSize];
    display.translateUTF8ToBlocks(sender, entry.sender, sizeof(sender));
    display.translateUTF8ToBlocks(message, entry.message, sizeof(message));

    display.setTextSize(2);
    if (!entry.read)
      display.fillRect(x, y + 9, 3, 3);

    display.drawTextLeftAlign(x + 6, y, message);
  }

public:
  MsgPage(UIViewModel* model) : UIPage(model) {
    _list.setRenderer(renderMessageItem, this);
  }

  const uint8_t* getIcon() override {
    return icon_msg;
  }

  void renderPreview(DisplayDriver& display) override {
    _list.setCount(static_cast<uint8_t>(_model->getMsgCount()));
    _list.render(display);
  }

  void activate() override {
    _list.setCount(static_cast<uint8_t>(_model->getMsgCount()));
    _list.reset();
    _model->renderAfter(0);
  }

  bool handleInput(char c) override {
    if (_list.handleInput(c))
      return true;

    if (c == KEY_ENTER) {
      if (_list.getCount() != 0)
        _model->gotoMsgViewer(_list.getSelected());
      return true;
    }

    return false;
  }
};

class ContactPage : public UIPage {
public:
  ContactPage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_contact;
  }

  void renderPreview(DisplayDriver& display) override {
  }
};

class ChannelPage : public UIPage {
public:
  ChannelPage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_channel;
  }

  void renderPreview(DisplayDriver& display) override {
  }
};

class RadioPage : public UIPage {
public:
  RadioPage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_radio;
  }

  void renderPreview(DisplayDriver& display) override {
    char tmp[40];
    auto details = _model->getRadioDetails();

    display.setTextSize(1);
    sprintf(tmp, "FQ: %06.3f", details.frequency);
    display.drawTextLeftAlign(3, 5, tmp);
    sprintf(tmp, "SF: %d", details.spreading_factor);
    display.drawTextLeftAlign(140, 5, tmp);

    sprintf(tmp, "BW: %03.2f", details.bandwidth);
    display.drawTextLeftAlign(3, 17, tmp);
    sprintf(tmp, "CR: %d", details.coding_factor);
    display.drawTextLeftAlign(140, 17, tmp);

    display.setTextSize(2);
    sprintf(tmp, "TX: %ddBm", details.transmit_power_dbm);
    display.drawTextLeftAlign(3, 40, tmp);
    sprintf(tmp, "Noise: %d", details.noise_floor_dbm);
    display.drawTextLeftAlign(140, 40, tmp);

    sprintf(tmp, "RSSI: %.1f", details.last_rssi_dbm);
    display.drawTextLeftAlign(3, 60, tmp);
    sprintf(tmp, "SNR: %.2f", details.last_snr_db);
    display.drawTextLeftAlign(140, 60, tmp);

    sprintf(tmp, "TX: %lu", details.packets_sent);
    display.drawTextLeftAlign(3, 80, tmp);
    sprintf(tmp, "RX: %lu", details.packets_received);
    display.drawTextLeftAlign(140, 80, tmp);
  }

  bool handleInput(char c) override {
    if (c != KEY_ENTER)
      return false;

    _model->resetRadioStats();
    return true;
  }
};

class GpsPage : public UIPage {
public:
  GpsPage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_gps;
  }

  void renderPreview(DisplayDriver& display) override {
    char tmp[30];
    auto pos = _model->getPosition();

    sprintf(tmp, "GPS: %s", pos.enabled ? "on" : "off");
    display.drawTextLeftAlign(3, 5, tmp);

    sprintf(tmp, "Fix: %s", pos.has_fix ? "yes" : "no");
    display.drawTextLeftAlign(120, 5, tmp);

    sprintf(tmp, "Sats: %d", pos.satellites);
    display.drawTextLeftAlign(3, 25, tmp);

    sprintf(tmp, "Elev: %.1f", pos.elevation);
    display.drawTextLeftAlign(120, 25, tmp);

    sprintf(tmp, "%.4f, %.4f", pos.latitude, pos.longitude);
    display.setTextSize(3);
    display.drawTextCentered(display.width() / 2, 60, tmp);
  }

  bool handleInput(char c) override {
    if (c != KEY_ENTER)
      return false;

    static const char* options[] = { "Enable", "Disable" };
    _model->prompt("GPS Sensor", options, 2, onGpsPrompt, _model);
    return true;
  }

private:
  static void onGpsPrompt(void* context, int result) {
    if (result < 0)
      return;

    auto* model = static_cast<UIViewModel*>(context);
    model->setGpsEnabled(result == 0);
  }
};

class ClockPage : public UIPage {
private:
  _7Seg _segs[6] {
    _7Seg(10, 40),
    _7Seg(45, 40),
    _7Seg(90, 40),
    _7Seg(125, 40),
    _7Seg(170, 40),
    _7Seg(205, 40)
  };

public:
  ClockPage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_clock;
  }

  void renderPreview(DisplayDriver& display) override {
    char tmp[30];
    auto dt = _model->getDateTime();
    auto center_x = display.width() / 2;

    _segs[0].set(dt.hour / 10);
    _segs[1].set(dt.hour % 10);
    _segs[2].set(dt.minute / 10);
    _segs[3].set(dt.minute % 10);
    _segs[4].set(dt.second / 10);
    _segs[5].set(dt.second % 10);

    for (auto seg : _segs)
      seg.render(display);

    // Blinking dots.
    if (dt.second % 2) {
      display.fillRect(79, 50, 3, 3);
      display.fillRect(79, 75, 3, 3);
      display.fillRect(159, 50, 3, 3);
      display.fillRect(159, 75, 3, 3);
    }

    sprintf(tmp, "%d/%d/%d", dt.month, dt.day, dt.year);
    display.drawTextCentered(center_x, 5, tmp);
  }
};

class PowerPage : public UIPage {
public:
  PowerPage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_power;
  }

  void renderPreview(DisplayDriver& display) override {
    char tmp[30];
    auto total_min = _model->getUptimeMin();
    auto hours = total_min / 60;
    auto minutes = total_min % 60;
    auto center_x = display.width() / 2;

    if (hours > 0)
      sprintf(tmp, "Uptime: %dh %dm", hours, minutes);
    else
      sprintf(tmp, "Uptime: %dm", minutes);

    display.setTextSize(3);
    display.drawTextCentered(center_x, 40, tmp);

    display.setTextSize(2);
    display.drawTextCentered(center_x, 70, _model->getFirmwareVersion());
  }

  bool handleInput(char c) override {
    if (c != KEY_ENTER)
      return false;

    static const char* options[] = { "Yes", "No" };
    _model->prompt("Shutdown?", options, 2, onShutdownPrompt, _model);
    return true;
  }

private:
  static void onShutdownPrompt(void* context, int result) {
    if (result == 0 && context) {
      static_cast<UIViewModel*>(context)->shutdown(false);
    }
  }
};
