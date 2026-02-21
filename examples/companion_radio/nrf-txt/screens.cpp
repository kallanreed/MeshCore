#include "screens.h"
#include "../MyMesh.h"
#include "keys.h"
#include "shared.h"
#include "ui_task.h"
#include <stdio.h>
#include <string.h>

namespace {
uint8_t safeStrLen(const char* text, uint8_t max_len) {
  if (!text)
    return 0;

  for (uint8_t i = 0; i < max_len; i++) {
    if (text[i] == 0)
      return i;
  }

  return max_len;
}

// TODO: Review
void drawWrappedText(
  DisplayDriver& display,
  const char* text,
  uint8_t length,
  int x,
  int y,
  int max_width,
  int max_height,
  int line_height,
  uint8_t cursor,
  int* cursor_x,
  int* cursor_y) {
  char line[64] = {};
  uint8_t line_len = 0;
  bool cursor_set = false;
  int cursor_draw_x = x;
  int cursor_draw_y = y;

  auto flush_line = [&](bool force) {
    if (line_len == 0 && !force)
      return;

    display.drawTextLeftAlign(x, y, line);
    y += line_height;
    line_len = 0;
    line[0] = 0;
  };

  for (uint8_t i = 0; i <= length; i++) {
    if (!cursor_set && i == cursor) {
      cursor_draw_x = x + display.getTextWidth(line);
      cursor_draw_y = y;
      cursor_set = true;
    }

    if (i == length)
      break;

    if (line_len + 1 >= sizeof(line)) {
      flush_line(false);
      if (y + line_height > max_height)
        break;
    }

    char ch[2] = { text[i], 0 };
    char next_line[64];
    if (line_len + 1 >= sizeof(next_line)) {
      next_line[0] = 0;
    } else if (line_len == 0) {
      snprintf(next_line, sizeof(next_line), "%s", ch);
    } else {
      snprintf(next_line, sizeof(next_line), "%s%s", line, ch);
    }

    bool needs_wrap = display.getTextWidth(next_line) > max_width;
    if (needs_wrap) {
      flush_line(false);
      if (y + line_height > max_height)
        break;
    }

    line[line_len++] = text[i];
    line[line_len] = 0;
  }

  flush_line(true);

  if (!cursor_set) {
    cursor_draw_x = x;
    cursor_draw_y = y;
  }

  if (cursor_x)
    *cursor_x = cursor_draw_x;
  if (cursor_y)
    *cursor_y = cursor_draw_y;
}
}  // namespace

// --- SplashScreen ---
SplashScreen::SplashScreen(UIViewModel* model) : _model(model) {
  auto ver = FIRMWARE_VERSION;
  auto len = strlen(ver);
  if (len >= sizeof(_version_info))
    len = sizeof(_version_info) - 1;

  memcpy(_version_info, ver, len);
  _version_info[len] = 0;

  dismiss_after = millis() + 3000;
}

int SplashScreen::render(DisplayDriver& display) {
  // Meshcore logo
  int logo_width = 128;
  int logo_height = 13;
  int mid_x = (display.width() - logo_width) / 2;
  display.drawXbm(mid_x, 35, meshcore_logo, logo_width, logo_height);

  // Version info
  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(2);
  display.drawTextCentered(display.width() / 2, 60, _version_info);

  display.setTextSize(1);
  display.drawTextCentered(display.width() / 2, 96, FIRMWARE_BUILD_DATE);

  return 1000;
}

void SplashScreen::poll() {
  if (millis() >= dismiss_after)
    _model->gotoHome();
}

// --- MsgViewer ---
MsgViewer::MsgViewer(UIViewModel* model) : _model(model) {
}

void MsgViewer::setMessageInternal(const MessageEntry& message) {
  _message = message;
  _has_message = true;
  _model->markMessageReadById(_message.timestamp_ms);
}

void MsgViewer::setMessage(const MessageEntry& message, MessageScope scope) {
  _scope = scope;
  setMessageInternal(message);
}

int MsgViewer::render(DisplayDriver& display) {
  constexpr int refresh = 10 * 1000;
  display.setTextSize(2);
  display.setColor(DisplayDriver::LIGHT);

  if (!_has_message) {
    display.drawTextLeftAlign(display.width() / 2, 60, "No messages");
    return refresh;
  }

  display.drawTextLeftAlign(2, 2, _message.sender);

  char age[12];
  formatAgeMillis(age, sizeof(age), _message.timestamp_ms);
  display.drawTextRightAlign(display.width() - 2, 2, age);

  display.drawRect(0, 20, display.width(), 1);

  display.setCursor(2, 24);
  display.printWordWrap(_message.message, display.width() - 4);

  return refresh;
}

bool MsgViewer::handleInput(char c) {
  if (isAnyKey(c, KeyCode::ESC, KeyCode::LEFT, KeyCode::ENTER)) {
    _model->gotoPrevious();
    return true;
  }

  bool handled = false;

  if (isKey(c, KeyCode::UP)) {
    MessageEntry previous{};
    if (_model->getPreviousMessage(_scope, _message, &previous)) {
      setMessageInternal(previous);
      handled = true;
    }
  } else if (isKey(c, KeyCode::DOWN)) {
    MessageEntry next{};
    if (_model->getNextMessage(_scope, _message, &next)) {
      setMessageInternal(next);
      handled = true;
    }
  }

  if (handled)
    _model->renderAfter(0);

  return handled;
}

// --- ThreadScreen ---
ThreadScreen::ThreadScreen(UIViewModel* model) : _model(model) {
  _list.setModel(model);
}

void ThreadScreen::onThreadText(void* context, const char* text) {
  auto* screen = static_cast<ThreadScreen*>(context);
  if (!screen || !text)
    return;

  if (screen->_is_contact) {
    screen->_model->sendContactMessage(screen->_target, text);
  } else {
    screen->_model->sendChannelMessage(screen->_target, text);
  }
}

void ThreadScreen::setContact(uint8_t contact_index) {
  _is_contact = true;
  _target = contact_index;
  _list.setContact(contact_index);
  auto name = _model->getContactName(contact_index);
  snprintf(_title, sizeof(_title), "Contact: %s", name ? name : "Contact");
}

void ThreadScreen::setChannel(uint8_t channel_index) {
  _is_contact = false;
  _target = channel_index;
  _list.setChannel(channel_index);
  auto name = _model->getChannelName(channel_index);
  snprintf(_title, sizeof(_title), "Channel: %s", name ? name : "Channel");
}

void ThreadScreen::refresh() {
  _list.refresh();
  _list.reset();
  auto count = _list.getCount();
  bool found_unread = _list.selectFirstUnread();
  if (!found_unread && count > 0)
    _list.setSelected(static_cast<uint8_t>(count - 1));
}

void ThreadScreen::activate() {
  refresh();
}

int ThreadScreen::render(DisplayDriver& display) {
  display.setTextSize(2);
  display.setColor(DisplayDriver::LIGHT);
  char title[sizeof(_title)];
  display.translateUTF8ToBlocks(title, _title, sizeof(title));
  display.drawTextLeftAlign(2, 2, title);
  display.drawRect(0, 20, display.width(), 1);
  if (_list.getCount() == 0) {
    display.drawTextCentered(display.width() / 2, 60, "No messages");
  } else {
    _list.render(display);
  }
  return 1000;
}

bool ThreadScreen::handleInput(char c) {
  if (isAnyKey(c, KeyCode::ESC, KeyCode::LEFT)) {
    _model->gotoHome();
    return true;
  }

  if (isKey(c, KeyCode::FN_ENTER)) {
    _text[0] = 0;
    char title[48];
    if (_is_contact) {
      auto name = _model->getContactName(_target);
      snprintf(title, sizeof(title), "Send to %s", name ? name : "Contact");
    } else {
      auto name = _model->getChannelName(_target);
      snprintf(title, sizeof(title), "Send to %s", name ? name : "Channel");
    }
    _model->promptText(title, _text, sizeof(_text), onThreadText, this);
    return true;
  }

  if (isKey(c, KeyCode::FN_R)) {
    if (_is_contact) {
      _model->markMessagesReadForContact(_target);
    } else {
      _model->markMessagesReadForChannel(_target);
    }
    _model->renderAfter(0);
    return true;
  }

  if (isKey(c, KeyCode::ENTER)) {
    auto selected = _list.getSelected();
    MessageEntry entry{};
    bool found = false;
    if (_is_contact) {
      found = _model->getMessagesForContact(_target, selected, 1, &entry) == 1;
    } else {
      found = _model->getMessagesForChannel(_target, selected, 1, &entry) == 1;
    }
    if (found) {
      _model->gotoMsgViewer(entry, MessageScope::thread);
    }
    return true;
  }

  if (_list.handleInput(c)) {
    _model->renderAfter(0);
    return true;
  }

  return false;
}

// --- TextInputScreen ---
TextInputScreen::TextInputScreen(UIViewModel* model)
  : _model(model) {
}

void TextInputScreen::begin(
  const char* title,
  char* buffer,
  uint8_t capacity,
  TextInputCallback callback,
  void* context) {
  if (title) {
    strncpy(_title, title, sizeof(_title));
    _title[sizeof(_title) - 1] = 0;
  } else {
    _title[0] = 0;
  }
  _buffer = buffer;
  _capacity = capacity;
  _callback = callback;
  _context = context;

  if (_capacity == 0) {
    _length = 0;
    _cursor = 0;
    return;
  }

  _length = safeStrLen(_buffer, static_cast<uint8_t>(_capacity - 1));
  _buffer[_length] = 0;
  _cursor = _length;
}

int TextInputScreen::render(DisplayDriver& display) {
  constexpr int refresh = 1000;
  display.setTextSize(2);
  display.setColor(DisplayDriver::LIGHT);
  const int margin = 2;
  int text_y = margin;

  if (!_buffer || _capacity == 0) {
    display.drawTextLeftAlign(margin, margin, "No buffer");
    return refresh;
  }

  if (_title[0] != 0) {
    display.drawTextLeftAlign(margin, margin, _title);
    display.drawRect(0, 20, display.width(), 1);
    text_y = 24;
  }

  int cursor_x = 0;
  int cursor_y = 0;
  int line_height = 18;
  drawWrappedText(
    display,
    _buffer,
    _length,
    margin,
    text_y,
    display.width() - (margin * 2),
    display.height() - (margin * 2),
    line_height,
    _cursor,
    &cursor_x,
    &cursor_y);

  int underline_w = display.getTextWidth("_");
  if (_cursor < _length) {
    char ch[2] = { _buffer[_cursor], 0 };
    underline_w = display.getTextWidth(ch);
  }

  if (underline_w < 2) {
    underline_w = 2;
  }

  int underline_y = cursor_y + line_height - 2;
  display.fillRect(cursor_x, underline_y, underline_w, 2);

  display.setTextSize(1);
  char counter[16];
  snprintf(counter, sizeof(counter), "%u/%u", _length, _capacity);
  display.drawTextRightAlign(display.width() - margin, display.height() - margin - 8, counter);

  return refresh;
}

// TODO: Review
bool TextInputScreen::handleInput(char c) {
  if (!_buffer || _capacity == 0)
    return false;

  if (isKey(c, KeyCode::ESC)) {
    _model->gotoPrevious();
    return true;
  }

  bool handled = false;

  if (isKey(c, KeyCode::LEFT)) {
    if (_cursor > 0)
      _cursor--;
    handled = true;
  } else if (isKey(c, KeyCode::RIGHT)) {
    if (_cursor < _length)
      _cursor++;
    handled = true;
  } else if (isKey(c, KeyCode::ENTER)) {
    _buffer[_length] = 0;
    if (_callback)
      _callback(_context, _buffer);
    _model->gotoPrevious();
    return true;
  } else if (c == 8 || c == 127) {
    if (_cursor == 0)
      return true;

    memmove(_buffer + _cursor - 1, _buffer + _cursor, _length - _cursor);
    _cursor--;
    _length--;
    _buffer[_length] = 0;
    handled = true;
  } else if (c < 32 || c > 126) {
    return false;
  } else if (_length + 1 >= _capacity) {
    handled = true;
  } else {
    if (_cursor < _length) {
      memmove(_buffer + _cursor + 1, _buffer + _cursor, _length - _cursor);
    }

    _buffer[_cursor] = c;
    _cursor++;
    _length++;
    _buffer[_length] = 0;
    handled = true;
  }

  if (handled)
    _model->renderAfter(0);

  return handled;
}

// --- Page Instances ---
extern UITask ui_task;
static UIViewModel* view_model = &ui_task;
static HomePage home_page = HomePage(view_model);
static AdvertPage advert_page = AdvertPage(view_model);
static ContactPage contact_page = ContactPage(view_model);
static ChannelPage channel_page = ChannelPage(view_model);
static RadioPage radio_page = RadioPage(view_model);
static GpsPage gps_page = GpsPage(view_model);
static ClockPage clock_page = ClockPage(view_model);
static PowerPage power_page = PowerPage(view_model);

// --- HomeScreen ---
HomeScreen::HomeScreen(UIViewModel* model) : _model(model) {
  _pages[0] = &home_page;
  _pages[1] = &contact_page;
  _pages[2] = &channel_page;
  _pages[3] = &advert_page;
  _pages[4] = &radio_page;
  _pages[5] = &gps_page;
  _pages[6] = &clock_page;
  _pages[7] = &power_page;

  current()->activate();
}

int HomeScreen::render(DisplayDriver& display) {
  display.setColor(DisplayDriver::LIGHT);
  auto current = _pages[_page];
  current->renderPreview(display);

  // Draw page selector
  for (auto i = 0u; i < _pages.size(); i++) {
    auto page = _pages[i];
    display.drawXbm(2 + (i * 20), 118, page->getIcon(), 8, 8, 2);
  }

  // Highlight selected.
  display.setColor(DisplayDriver::INVERSE);
  display.fillRect(1 + (20 * _page), 117, 18, 18);
  display.setColor(DisplayDriver::LIGHT);

  // Draw battery.
  _batt.render(display, _model->getBatteryPercent());

  return 1000;
}

bool HomeScreen::handleInput(char c) {
  bool handled = current()->handleInput(c);

  if (!handled) {
    if (isKey(c, KeyCode::LEFT)) {
      _page = (_pages.size() + _page - 1) % _pages.size();
      current()->activate();
      handled = true;
    } else if (isKey(c, KeyCode::RIGHT)) {
      _page = (_page + 1) % _pages.size();
      current()->activate();
      handled = true;
    }
  }

  if (handled)
    _model->renderAfter(0);

  return handled;
}

void HomeScreen::poll() {}

void HomeScreen::activate() {
  current()->activate();
}
