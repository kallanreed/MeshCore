#pragma once

#include <stdio.h>
#include <string.h>
#include <helpers/ui/UIScreen.h>
#include "icons.h"
#include "keys.h"
#include "shared.h"
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

    if (isKey(c, KeyCode::UP)) {
      if (_count > 0)
        _selected = (_selected + _count - 1) % _count;
    } else if (isKey(c, KeyCode::DOWN)) {
      if (_count > 0)
        _selected = (_selected + 1) % _count;
    } else if (isKey(c, KeyCode::ENTER)) {
      finish(_selected);
    } else if (isKey(c, KeyCode::ESC)) {
      finish(-1);
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

  void setSelected(uint8_t selected) {
    if (_count == 0) {
      reset();
      return;
    }

    if (selected >= _count)
      selected = _count - 1;

    _selected = selected;
    clampTop();
  }

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

    bool handled = false;
    if (isKey(c, KeyCode::UP)) {
      _selected = (_selected + _count - 1) % _count;
      clampTop();
      handled = true;
    } else if (isKey(c, KeyCode::DOWN)) {
      _selected = (_selected + 1) % _count;
      clampTop();
      handled = true;
    }

    return handled;
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

    if (_count == 0 || visible == 0 || _count <= visible)
      return;

    // Draw scrollbar.
    display.setColor(DisplayDriver::DARK);
    display.fillRect(_x + _w - 2, _y, 2, _h);

    // Scale indicator height based on the visible/total ratio.
    int indicator_y = _y;
    int indicator_h = (_h * visible) / _count;
    if (indicator_h < 1)
      indicator_h = 1;

    // Map the top row to the indicator track height.
    if (_count > visible) {
      int max_offset = _count - visible;
      int track = _h - indicator_h;
      indicator_y = _y + (track * _top) / max_offset;
    }

    display.setColor(DisplayDriver::LIGHT);
    display.fillRect(_x + _w - 1, indicator_y, 1, indicator_h);
  }
};

class MessageBuffer {
  MessageEntry _entries[kMessageBufferSize] = {};
  uint8_t _head = 0;
  uint8_t _count = 0;

  uint8_t toIndex(uint8_t offset) const {
    uint8_t oldest = 0;
    if (_count == kMessageBufferSize)
      oldest = _head;

    return (oldest + offset) % kMessageBufferSize;
  }

  bool matchesContact(const MessageEntry& entry, const uint8_t* prefix) const {
    if (!prefix || entry.kind != MessageKind::contact)
      return false;
    return memcmp(entry.contact_prefix, prefix, sizeof(entry.contact_prefix)) == 0;
  }

  bool matchesChannel(const MessageEntry& entry, uint8_t channel_index) const {
    return entry.kind == MessageKind::channel && entry.channel_index == channel_index;
  }

  bool matchesThread(const MessageEntry& entry, const MessageEntry& current) const {
    if (current.kind == MessageKind::contact)
      return matchesContact(entry, current.contact_prefix);
    if (current.kind == MessageKind::channel)
      return matchesChannel(entry, current.channel_index);
    return false;
  }

  bool matchesScope(MessageScope scope, const MessageEntry& entry, const MessageEntry& current) const {
    if (scope == MessageScope::all)
      return true;
    return matchesThread(entry, current);
  }

  bool findMessageIndexById(uint32_t message_id, uint8_t* out_index) const {
    if (!out_index)
      return false;

    for (uint8_t i = 0; i < _count; i++) {
      auto& entry = _entries[toIndex(i)];
      if (entry.timestamp_ms == message_id) {
        *out_index = i;
        return true;
      }
    }
    return false;
  }

public:
  uint8_t getCount() const { return _count; }

  uint8_t addMessage(
    uint32_t timestamp_ms,
    const char* sender,
    const char* message,
    MessageKind kind,
    const uint8_t* contact_prefix,
    uint8_t channel_index,
    MessageDirection direction) {
    auto* entry = &_entries[_head];
    entry->read = direction == MessageDirection::outgoing;
    entry->timestamp_ms = timestamp_ms;
    entry->kind = kind;
    entry->direction = direction;
    entry->channel_index = channel_index;
    if (contact_prefix) {
      memcpy(entry->contact_prefix, contact_prefix, sizeof(entry->contact_prefix));
    } else {
      memset(entry->contact_prefix, 0, sizeof(entry->contact_prefix));
    }

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
    return _count == 0 ? 0 : static_cast<uint8_t>(_count - 1);
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

  void markReadById(uint32_t message_id) {
    uint8_t index = 0;
    if (!findMessageIndexById(message_id, &index))
      return;
    _entries[toIndex(index)].read = true;
  }

  void markAllRead() {
    for (uint8_t i = 0; i < _count; i++) {
      _entries[toIndex(i)].read = true;
    }
  }

  uint8_t getUnreadCount() const {
    uint8_t unread = 0;
    for (uint8_t i = 0; i < _count; i++) {
      if (!_entries[toIndex(i)].read)
        unread++;
    }
    return unread;
  }

  uint8_t getCountForContact(const uint8_t* prefix) const {
    if (!prefix)
      return 0;
    uint8_t total = 0;
    for (uint8_t i = 0; i < _count; i++) {
      if (matchesContact(_entries[toIndex(i)], prefix))
        total++;
    }
    return total;
  }

  uint8_t getCountForChannel(uint8_t channel_index) const {
    uint8_t total = 0;
    for (uint8_t i = 0; i < _count; i++) {
      if (matchesChannel(_entries[toIndex(i)], channel_index))
        total++;
    }
    return total;
  }

  uint8_t getUnreadCountForContact(const uint8_t* prefix) const {
    if (!prefix)
      return 0;
    uint8_t unread = 0;
    for (uint8_t i = 0; i < _count; i++) {
      auto& entry = _entries[toIndex(i)];
      if (matchesContact(entry, prefix) && !entry.read)
        unread++;
    }
    return unread;
  }

  uint8_t getUnreadCountForChannel(uint8_t channel_index) const {
    uint8_t unread = 0;
    for (uint8_t i = 0; i < _count; i++) {
      auto& entry = _entries[toIndex(i)];
      if (matchesChannel(entry, channel_index) && !entry.read)
        unread++;
    }
    return unread;
  }

  uint8_t getMessagesForContact(
    uint8_t offset,
    uint8_t count,
    MessageEntry* out,
    const uint8_t* prefix) const {
    if (!out || count == 0 || !prefix)
      return 0;

    uint8_t matched = 0;
    uint8_t copied = 0;
    for (uint8_t i = 0; i < _count && copied < count; i++) {
      auto& entry = _entries[toIndex(i)];
      if (!matchesContact(entry, prefix))
        continue;

      if (matched >= offset) {
        out[copied++] = entry;
      }
      matched++;
    }
    return copied;
  }

  uint8_t getMessagesForChannel(
    uint8_t offset,
    uint8_t count,
    MessageEntry* out,
    uint8_t channel_index) const {
    if (!out || count == 0)
      return 0;

    uint8_t matched = 0;
    uint8_t copied = 0;
    for (uint8_t i = 0; i < _count && copied < count; i++) {
      auto& entry = _entries[toIndex(i)];
      if (!matchesChannel(entry, channel_index))
        continue;

      if (matched >= offset) {
        out[copied++] = entry;
      }
      matched++;
    }
    return copied;
  }

  void markAllReadForContact(const uint8_t* prefix) {
    if (!prefix)
      return;
    for (uint8_t i = 0; i < _count; i++) {
      auto idx = toIndex(i);
      if (matchesContact(_entries[idx], prefix))
        _entries[idx].read = true;
    }
  }

  void markAllReadForChannel(uint8_t channel_index) {
    for (uint8_t i = 0; i < _count; i++) {
      auto idx = toIndex(i);
      if (matchesChannel(_entries[idx], channel_index))
        _entries[idx].read = true;
    }
  }

  bool getPreviousMessage(MessageScope scope, const MessageEntry& current, MessageEntry* out) const {
    return getAdjacentMessage(scope, current, false, out);
  }

  bool getNextMessage(MessageScope scope, const MessageEntry& current, MessageEntry* out) const {
    return getAdjacentMessage(scope, current, true, out);
  }

  bool getAdjacentMessage(MessageScope scope, const MessageEntry& current, bool forward, MessageEntry* out) const {
    if (!out)
      return false;

    uint8_t start = 0;
    if (!findMessageIndexById(current.timestamp_ms, &start))
      return false;

    int16_t step = forward ? 1 : -1;
    int16_t i = static_cast<int16_t>(start) + step;
    while (i >= 0 && i < _count) {
      auto& entry = _entries[toIndex(static_cast<uint8_t>(i))];
      if (matchesScope(scope, entry, current)) {
        *out = entry;
        return true;
      }
      i += step;
    }
    return false;
  }
};

enum class MessageListMode : uint8_t {
  all,
  contact,
  channel
};

class MessageList {
  ScrollList _list;
  UIViewModel* _model = nullptr;
  MessageListMode _mode = MessageListMode::all;
  uint8_t _target = 0;

  uint8_t getTotalCount() const {
    if (!_model)
      return 0;
    switch (_mode) {
      case MessageListMode::contact:
        return _model->getMsgCountForContact(_target);
      case MessageListMode::channel:
        return _model->getMsgCountForChannel(_target);
      case MessageListMode::all:
      default:
        return static_cast<uint8_t>(_model->getMsgCount());
    }
  }

  bool getMessage(uint8_t index, MessageEntry& out) const {
    if (!_model)
      return false;
    switch (_mode) {
      case MessageListMode::contact:
        return _model->getMessagesForContact(_target, index, 1, &out) == 1;
      case MessageListMode::channel:
        return _model->getMessagesForChannel(_target, index, 1, &out) == 1;
      case MessageListMode::all:
      default:
        return _model->getMessages(index, 1, &out) == 1;
    }
  }

  static void renderMessageItem(
    DisplayDriver& display,
    uint8_t index,
    int x,
    int y,
    int w,
    int h,
    void* context) {
    auto* list = static_cast<MessageList*>(context);
    if (!list)
      return;

    MessageEntry entry{};
    if (!list->getMessage(index, entry))
      return;

    char tmp[kMessageTextSize + 4];
    if (entry.direction == MessageDirection::outgoing) {
      snprintf(tmp, sizeof(tmp), "> %s", entry.message);
    } else {
      snprintf(tmp, sizeof(tmp), "%s", entry.message);
    }

    display.setColor(DisplayDriver::DARK);
    display.fillRect(x, y, w, h);

    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(1);
    if (!entry.read)
      display.fillRect(x, y + 4, 3, 3);

    display.drawTextLeftAlign(x + 6, y - 2, tmp);
  }

public:
  MessageList(int x, int y, int w, int h, uint8_t row_h)
    : _list(x, y, w, h, row_h) {
    _list.setRenderer(renderMessageItem, this);
  }

  void setModel(UIViewModel* model) { _model = model; }

  void setModeAll() { _mode = MessageListMode::all; }
  void setContact(uint8_t contact_index) {
    _mode = MessageListMode::contact;
    _target = contact_index;
  }
  void setChannel(uint8_t channel_index) {
    _mode = MessageListMode::channel;
    _target = channel_index;
  }

  void refresh() {
    _list.setCount(getTotalCount());
    if (_list.getCount() == 0)
      _list.reset();
  }

  void reset() { _list.reset(); }
  void setSelected(uint8_t selected) { _list.setSelected(selected); }
  uint8_t getSelected() const { return _list.getSelected(); }
  uint8_t getCount() { return _list.getCount(); }

  bool handleInput(char c) { return _list.handleInput(c); }

  void render(DisplayDriver& display) { _list.render(display); }

  bool selectFirstUnread() {
    auto count = getTotalCount();
    for (uint8_t offset = 0; offset < count; offset++) {
      MessageEntry entry{};
      if (!getMessage(offset, entry))
        continue;
      if (!entry.read) {
        _list.setSelected(offset);
        return true;
      }
    }
    return false;
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

  static void onOptionsSelected(void* context, int result) {
    auto* model = static_cast<UIViewModel*>(context);
    if (!model || result < 0)
      return;

    switch (result) {
      case 0:
        model->toggleBuzzer();
        break;
      case 1:
        model->toggleScreenInvert();
        break;
      default:
        break;
    }
  }

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
    char name[32];
    display.translateUTF8ToBlocks(name, _model->getNodeName(), sizeof(name));
    display.drawTextCentered(center_x, 12, name);

    display.setTextSize(3);
    sprintf(_text, "Unread: %lu", _model->getUnreadMsgCount());
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
    if (!isKey(c, KeyCode::ENTER))
      return false;

    static const char* options[] = { "Toggle Buzzer", "Toggle Invert" };
    _model->prompt("Options", options, 2, onOptionsSelected, _model);
    return true;
  }
};

class AdvertPage : public UIPage {
  ScrollList _list = ScrollList(0, 0, 240, 116, 20);
  RecentAdvertEntry _entries[kRecentAdvertMax] = {};
  uint8_t _count = 0;
  RecentAdvertEntry _pending_entry = {};

  void refresh() {
    auto selected = _list.getSelected();
    _count = _model->getRecentAdverts(_entries, kRecentAdvertMax);
    _list.setCount(_count);
    if (_count == 0) {
      _list.reset();
      return;
    }

    if (selected < _count) {
      _list.setSelected(selected);
    } else {
      _list.setSelected(static_cast<uint8_t>(_count - 1));
    }
  }

  static void onAddPrompt(void* context, int result) {
    auto* page = static_cast<AdvertPage*>(context);
    if (!page)
      return;

    if (result == 0) {
      if (!page->_model->addRecentAdvertContact(page->_pending_entry)) {
        static const char* options[] = { "OK" };
        page->_model->prompt("Add Failed", options, 1, nullptr, nullptr);
      }
    }
  }

  static void renderAdvertItem(
    DisplayDriver& display,
    uint8_t index,
    int x,
    int y,
    int w,
    int h,
    void* context) {
    auto* page = static_cast<AdvertPage*>(context);
    if (!page || index >= page->_count)
      return;

    auto& entry = page->_entries[index];
    char name[kRecentAdvertNameSize];
    char age[12];
    display.translateUTF8ToBlocks(name, entry.name, sizeof(name));
    uint32_t now = page->_model->getRtcSeconds();
    uint32_t age_sec = now >= entry.recv_timestamp ? (now - entry.recv_timestamp) : 0;
    formatAgeSeconds(age, sizeof(age), age_sec);

    display.setTextSize(2);
    int age_width = display.getTextWidth(age);
    int max_name_width = w - age_width - 2;
    if (max_name_width < 0)
      max_name_width = 0;
    display.drawTextEllipsized(x, y, max_name_width, name);
    display.drawTextRightAlign(x + w - 1, y, age);
  }

public:
  AdvertPage(UIViewModel* model) : UIPage(model) {
    _list.setRenderer(renderAdvertItem, this);
  }

  const uint8_t* getIcon() override {
    return icon_recent;
  }

  void renderPreview(DisplayDriver& display) override {
    refresh();
    _list.render(display);
  }

  void activate() override {
    refresh();
    _list.reset();
    if (_count > 0)
      _list.setSelected(0);
  }

  bool handleInput(char c) override {
    if (_list.handleInput(c))
      return true;

    if (!isKey(c, KeyCode::ENTER))
      return false;

    if (_count == 0)
      return true;

    auto selected = _list.getSelected();
    if (selected >= _count)
      return true;

    auto& entry = _entries[selected];
    if (_model->hasContact(entry.pub_key)) {
      static const char* options[] = { "OK" };
      _model->prompt("Already Added", options, 1, nullptr, nullptr);
      return true;
    }

    _pending_entry = entry;
    static const char* options[] = { "Add", "Cancel" };
    _model->prompt("Add Contact?", options, 2, onAddPrompt, this);
    return true;
  }
};

class ContactPage : public UIPage {
public:
  ScrollList _list = ScrollList(0, 0, 240, 116, 20);
  uint8_t _selected_contact_index = 0;
  uint8_t _indexes[MAX_CONTACTS] = {}; // contact indexes for UI rows
  uint8_t _index_count = 0;

  void refreshContacts() {
    _index_count = _model->getContactIndexes(_indexes, sizeof(_indexes));
  }

  static void renderContactItem(
    DisplayDriver& display,
    uint8_t index,
    int x,
    int y,
    int w,
    int h,
    void* context) {
    auto* page = static_cast<ContactPage*>(context);
    if (!page)
      return;

    if (index >= page->_index_count)
      return;

    auto contact_index = page->_indexes[index];
    auto name = page->_model->getContactName(contact_index);
    if (!name)
      return;

    char name_buf[kRecentAdvertNameSize];
    display.translateUTF8ToBlocks(name_buf, name, sizeof(name_buf));

    display.setTextSize(2);
    if (page->_model->getUnreadCountForContact(contact_index) > 0)
      display.fillRect(x, y + 9, 3, 3);
    display.drawTextLeftAlign(x + 6, y, name_buf);
  }

  ContactPage(UIViewModel* model) : UIPage(model) {
    _list.setRenderer(renderContactItem, this);
  }

  const uint8_t* getIcon() override {
    return icon_contact;
  }

  void renderPreview(DisplayDriver& display) override {
    _list.render(display);
  }

  void activate() override {
    refreshContacts();
    _list.setCount(_index_count);
    _list.reset();
    if (_list.getCount() > 0)
      _list.setSelected(0);
  }

  bool handleInput(char c) override {
    if (_list.handleInput(c))
      return true;

    if (!isKey(c, KeyCode::ENTER))
      return false;

    if (_list.getCount() == 0)
      return true;

    if (_list.getSelected() < _index_count) {
      _selected_contact_index = _indexes[_list.getSelected()];
    }
    _model->gotoContactThread(_selected_contact_index);
    return true;
  }
};

class ChannelPage : public UIPage {
  ScrollList _list = ScrollList(0, 0, 240, 116, 20);
  uint8_t _selected_channel_index = 0;
  uint8_t _indexes[MAX_GROUP_CHANNELS] = {}; // channel indexes for UI rows
  uint8_t _index_count = 0;

  void refreshChannels() {
    _index_count = _model->getChannelIndexes(_indexes, sizeof(_indexes));
  }

  static void renderChannelItem(
    DisplayDriver& display,
    uint8_t index,
    int x,
    int y,
    int w,
    int h,
    void* context) {
    auto* page = static_cast<ChannelPage*>(context);
    if (index >= page->_index_count)
      return;

    auto channel_index = page->_indexes[index];
    auto name = page->_model->getChannelName(channel_index);
    if (!name)
      return;

    display.setTextSize(2);
    if (page->_model->getUnreadCountForChannel(channel_index) > 0)
      display.fillRect(x, y + 9, 3, 3);
    display.drawTextLeftAlign(x + 6, y, name);
  }

public:
  ChannelPage(UIViewModel* model) : UIPage(model) {
    _list.setRenderer(renderChannelItem, this);
  }

  const uint8_t* getIcon() override {
    return icon_channel;
  }

  void renderPreview(DisplayDriver& display) override {
    _list.render(display);
  }

  void activate() override {
    refreshChannels();
    _list.setCount(_index_count);
    _list.reset();
    if (_list.getCount() > 0)
      _list.setSelected(0);
  }

  bool handleInput(char c) override {
    if (_list.handleInput(c))
      return true;

    if (!isKey(c, KeyCode::ENTER))
      return false;

    if (_list.getCount() == 0)
      return true;

    if (_list.getSelected() < _index_count) {
      _selected_channel_index = _indexes[_list.getSelected()];
      _model->gotoChannelThread(_selected_channel_index);
    }

    return true;
  }
};

class RadioPage : public UIPage {
public:
  RadioPage(UIViewModel* model) : UIPage(model) {}

  static void onOptionsSelected(void* context, int result) {
    auto* model = static_cast<UIViewModel*>(context);
    if (!model || result < 0)
      return;

    switch (result) {
      case 0:
        model->resetRadioStats();
        break;
      case 1:
        model->toggleBle();
        break;
      case 2:
        model->sendAdvert(false);
        break;
      case 3:
        model->sendAdvert(true);
        break;
      default:
        break;
    }
  }

  const uint8_t* getIcon() override {
    return icon_radio;
  }

  void renderPreview(DisplayDriver& display) override {
    char tmp[40];
    auto details = _model->getRadioDetails();

    if (_model->isBleEnabled())
      display.drawXbm(222, 2, icon_ble_16, 16, 16);

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
    if (!isKey(c, KeyCode::ENTER))
      return false;

    static const char* options[] = { "Reset Stats", "Toggle BLE", "0-Hop Advert", "Flood Advert" };
    _model->prompt("Radio Options", options, 4, onOptionsSelected, _model);
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
    if (!isKey(c, KeyCode::ENTER))
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

class DebugPage : public UIPage {
public:
  DebugPage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_shroom;
  }

  void renderPreview(DisplayDriver& display) override {
    static const char* lines10[] = {
      "😀 😂 😍 😢 😠 😮 😕 🤔 😎 😒",
      "👍 👎 ⚠ ⭐ ▶ ❓ 💬 💡 🛠 🤖",
      "🧭 👨 🎉 💰 🔒 🌿 🦆 🐶 👻"
    };
    static const char* lines16[] = {
      "😀 😂 😍 😢 😠 😮 😕 🤔 😎 😒",
      "👍 👎 ⚠ ⭐ ▶ ❓ 💬 💡 🛠 🤖",
      "🧭 👨 🎉 💰 🔒 🌿 🦆 🐶 👻"
    };
    char line[96];

    display.setTextSize(1);
    for (size_t i = 0; i < sizeof(lines10) / sizeof(lines10[0]); i++) {
      display.translateUTF8ToBlocks(line, lines10[i], sizeof(line));
      display.drawTextLeftAlign(4, 4 + (int)(i * 12), line);
    }

    display.setTextSize(2);
    for (size_t i = 0; i < sizeof(lines16) / sizeof(lines16[0]); i++) {
      display.translateUTF8ToBlocks(line, lines16[i], sizeof(line));
      display.drawTextLeftAlign(4, 50 + (int)(i * 20), line);
    }

    if (_invert) {
      display.setColor(DisplayDriver::INVERSE);
      display.fillRect(0, 0, 240, 115);
      display.setColor(DisplayDriver::LIGHT);
    }
  }

  bool handleInput(char c) override {
    if (!isKey(c, KeyCode::ENTER))
      return false;

    _invert = !_invert;
    return true;
  }

private:
  bool _invert = false;
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
    if (!isKey(c, KeyCode::ENTER))
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
