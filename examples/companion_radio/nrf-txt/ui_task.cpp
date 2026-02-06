#include "ui_task.h"

#include <stdio.h>
#include <string.h>

#include "../MyMesh.h"
#include <helpers/AdvertDataHelpers.h>
#include <helpers/ChannelDetails.h>
#include <helpers/TxtDataHelpers.h>
#include "screens.h"
#include "target.h"

constexpr uint32_t auto_off_ms = 15 * 1000;

// --- Private functions ---
void UITask::dispatchRender() {
  if (!_display || !_display->isOn())
    return;

  if (millis() < _next_render || !_curr)
    return;

  // TODO: alert handling. Make a control.
  _display->startFrame();
  auto delay_ms = _curr->render(*_display);
  if (_prompt.isActive()) {
    _prompt.render(*_display);
  }
  renderAfter(delay_ms);
  _display->endFrame();
}

void UITask::setCurrent(UIScreen* screen) {
  _curr = screen;
  _curr->activate();
  renderAfter(0);
}

 bool UITask::wakeScreen() {
  bool previously_on = true;
  if (!_display->isOn()) {
    previously_on = false;
    _display->turnOn();
  }
  renderAfter(0); // Refresh screen.
  _auto_off = millis() + auto_off_ms;
  return previously_on;
 }

void UITask::checkAutoOff() {
  if (_display->isOn() && _auto_off < millis())
    _display->turnOff();
}

// --- Public functions ---
void UITask::begin(
  DisplayDriver* display,
  SensorManager* sensors,
  NodePrefs* node_prefs)
{
  _display = display;
  _sensors = sensors;
  _node_prefs = node_prefs;

  _keyboard.begin();
  _buzzer.begin();
  _buzzer.quiet(_node_prefs->buzzer_quiet);

  wakeScreen();
  _ui_started_at = millis();

  _splash = new SplashScreen(this);
  _home = new HomeScreen(this);
  _msg_viewer = new MsgViewer(this);
  _text_input = new TextInputScreen(this);
  setCurrent(_splash);
}

// --- AbstractUITask ---
void UITask::msgRead(int msgcount) {
  // no-op
}

void UITask::newMsg(
  uint8_t path_len,
  const char* from_name,
  const char* text,
  int msgcount) {
  _message_buffer.addMessage(millis(), from_name ? from_name : "", text ? text : "");
  renderAfter(0);
}

void UITask::notify(UIEventType t) {
  switch(t) {
    case UIEventType::contactMessage:
      _buzzer.play("MsgRcv3:d=4,o=6,b=200:32e,32g,32b,16c7");
      break;
    case UIEventType::channelMessage:
      _buzzer.play("kerplop:d=16,o=6,b=120:32g#,32c#");
      break;
    case UIEventType::ack:
      _buzzer.play("ack:d=32,o=8,b=200:c,e");
      break;
    case UIEventType::roomMessage:
    case UIEventType::newContactMessage:
    case UIEventType::none:
    default:
      break;
  }
}

void UITask::loop() {
  auto kb = _keyboard.readKeyboard();
  if (kb) {
    if (!wakeScreen()) {
      // Screen was off, call activate to ready the page.
      _curr->activate();
    } else if (_prompt.isActive()) {
      _prompt.handleInput(kb);
    } else {
      _curr->handleInput(kb);
    }
  }

  if (_buzzer.isPlaying())
    _buzzer.loop();

  if (_curr)
    _curr->poll();

  dispatchRender();
  checkAutoOff();
}

// --- UIViewModel ---
uint32_t UITask::getMsgCount() {
  return _message_buffer.getCount();
}

uint32_t UITask::getUnreadMsgCount() {
  return _message_buffer.getUnreadCount();
}

bool UITask::isConnected() {
  return hasConnection();
}

bool UITask::isBuzzerEnabled() {
  return !_buzzer.isQuiet();
}

void UITask::prompt(
  const char* title,
  const char* const* items,
  uint8_t count,
  PromptCallback callback,
  void* context) {
  if (!callback || count == 0)
    return;

  _prompt.begin(title, items, count, callback, context);
  renderAfter(0);
}

void UITask::promptText(
  const char* title,
  char* buffer,
  uint8_t capacity,
  TextInputCallback callback,
  void* context) {
  if (!_text_input || !buffer || capacity == 0)
    return;

  static_cast<TextInputScreen*>(_text_input)->begin(
    title,
    buffer,
    capacity,
    callback,
    context);
  setCurrent(_text_input);
}

bool UITask::sendChannelMessage(uint8_t channel_index, const char* text) {
  if (!text)
    return false;

  auto len = strlen(text);
  if (len == 0)
    return false;

  ChannelDetails details;
  if (!the_mesh.getChannel(channel_index, details))
    return false;

  auto now = the_mesh.getRTCClock()->getCurrentTime();
  auto name = the_mesh.getNodeName();
  return the_mesh.sendGroupMessage(now, details.channel, name, text, len);
}

uint8_t UITask::getChannelSlots(uint8_t* slots, uint8_t max) {
  if (!slots || max == 0)
    return 0;

  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_GROUP_CHANNELS && count < max; i++) {
    ChannelDetails details;
    if (!the_mesh.getChannel(i, details))
      continue;

    bool has_secret = false;
    for (size_t j = 0; j < sizeof(details.channel.secret); j++) {
      if (details.channel.secret[j] != 0) {
        has_secret = true;
        break;
      }
    }

    if (!has_secret)
      continue;

    slots[count++] = i;
  }

  return count;
}

const char* UITask::getChannelName(uint8_t slot) {
  static ChannelDetails details;
  if (!the_mesh.getChannel(slot, details))
    return nullptr;

  // TODO: may need special case for "Public"
  return details.name;
}

static bool getChatContactBySlot(uint8_t slot, ContactInfo& out) {
  if (!the_mesh.getContactByIdx(slot, out))
    return false;
  return out.type == ADV_TYPE_CHAT;
}

uint8_t UITask::getContactSlots(uint8_t* slots, uint8_t max) {
  if (!slots || max == 0)
    return 0;

  uint8_t count = 0;
  auto total = the_mesh.getNumContacts();
  for (uint32_t i = 0; i < static_cast<uint32_t>(total) && count < max; i++) {
    ContactInfo contact{};
    if (!the_mesh.getContactByIdx(i, contact))
      continue;
    if (contact.type != ADV_TYPE_CHAT)
      continue;
    slots[count++] = static_cast<uint8_t>(i);
  }
  return count;
}

const char* UITask::getContactName(uint8_t slot) {
  static char name_buf[40];
  ContactInfo contact{};
  if (!getChatContactBySlot(slot, contact))
    return nullptr;

  if (contact.name[0]) {
    StrHelper::strncpy(name_buf, contact.name, sizeof(name_buf));
    return name_buf;
  }

  snprintf(
    name_buf,
    sizeof(name_buf),
    "ID:%02X%02X%02X%02X%02X%02X",
    contact.id.pub_key[0],
    contact.id.pub_key[1],
    contact.id.pub_key[2],
    contact.id.pub_key[3],
    contact.id.pub_key[4],
    contact.id.pub_key[5]);
  return name_buf;
}

bool UITask::sendContactMessage(uint8_t slot, const char* text) {
  if (!text)
    return false;

  auto len = strlen(text);
  if (len == 0)
    return false;

  ContactInfo contact{};
  if (!getChatContactBySlot(slot, contact))
    return false;

  auto now = the_mesh.getRTCClock()->getCurrentTime();
  uint32_t expected_ack = 0;
  uint32_t est_timeout = 0;
  auto result = the_mesh.sendMessage(contact, now, 0, text, expected_ack, est_timeout);
  return result != MSG_SEND_FAILED;
}

uint32_t UITask::getBlePin() {
  return the_mesh.getBLEPin();
}

bool UITask::isBleEnabled() {
  return isSerialEnabled();
}

void UITask::toggleBle() {
  if (isSerialEnabled()) {
    disableSerial();
  } else {
    enableSerial();
  }
}

uint32_t UITask::getUptimeMin() {
  auto uptime_millis = millis() - _ui_started_at;
  return uptime_millis / 1000 / 60;
}

void UITask::gotoMsgViewer(uint8_t offset) {
  static_cast<MsgViewer*>(_msg_viewer)->setOffset(offset);
  setCurrent(_msg_viewer);
}

void UITask::renderAfter(uint32_t delay_ms) {
  _next_render = millis() + delay_ms;
}

void UITask::shutdown(bool restart) {
  _keyboard.end();
  _buzzer.shutdown();

  // Give the buzzer some time to play.
  uint32_t buzzer_timer = millis();
  while (_buzzer.isPlaying() && (millis() - 2500) < buzzer_timer)
    _buzzer.loop();

  if (restart) {
    _board->reboot();
  } else {
    radio_driver.powerOff();
    _display->turnOff();
    _board->powerOff();
  }
}

void UITask::toggleBuzzer() {
  if (_buzzer.isQuiet()) {
    _buzzer.quiet(false);
    notify(UIEventType::ack);
  } else {
    _buzzer.quiet(true);
  }
  _node_prefs->buzzer_quiet = _buzzer.isQuiet();
  the_mesh.savePrefs();
  //showAlert(buzzer.isQuiet() ? "Buzzer: OFF" : "Buzzer: ON", 800);
  //_next_refresh = 0;
}

bool UITask::sendAdvert() {
  notify(UIEventType::ack);
  return the_mesh.advert();
}

void UITask::setGpsEnabled(bool enabled) {
  if (!_sensors)
    return;

  // TODO: Clean up.
  int num = _sensors->getNumSettings();
  for (int i = 0; i < num; i++) {
    if (strcmp(_sensors->getSettingName(i), "gps") == 0) {
      const char* value = enabled ? "1" : "0";
      MESH_DEBUG_PRINT("UITask::setGpsEnabled> ");
      _sensors->setSettingValue("gps", value);
      _node_prefs->gps_enabled = enabled ? 1 : 0;
      notify(UIEventType::ack);
      MESH_DEBUG_PRINTLN("GPS Enabled: %c", value);
      the_mesh.savePrefs();
      break;
    }
  }
}

Position UITask::getPosition() {
  Position p{};
  LocationProvider* nmea = sensors.getLocationProvider();

  if (nmea) {
    p.has_fix = nmea->isValid();
    p.latitude = nmea->getLatitude() / 1000000.0;
    p.longitude = nmea->getLongitude() / 1000000.0;
    p.elevation = nmea->getAltitude() / 1000.0; // m
    p.satellites = nmea->satellitesCount();
    p.enabled = nmea->isEnabled();
  }

  return p;
}

DateTime2 UITask::getDateTime() {
  DateTime2 out{};
  auto* _rtc = the_mesh.getRTCClock();
  if (!_rtc) {
    return out;
  }

  uint32_t epoch = _rtc->getCurrentTime();
  // TODO: TZ config with DST
  int32_t offset = -8 * 60 * 60;

  uint32_t seconds = epoch + offset;
  out.second = seconds % 60;
  seconds /= 60;
  out.minute = seconds % 60;
  seconds /= 60;
  out.hour = seconds % 24;
  uint32_t days = seconds / 24;

  // Magic AI code.
  int64_t z = static_cast<int64_t>(days) + 719468;
  int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  uint32_t doe = static_cast<uint32_t>(z - era * 146097); // [0, 146096]
  uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  int32_t year = static_cast<int32_t>(yoe) + static_cast<int32_t>(era) * 400;
  uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  uint32_t mp = (5 * doy + 2) / 153;
  uint32_t day = doy - (153 * mp + 2) / 5 + 1;
  uint32_t month = mp + (mp < 10 ? 3 : -9);
  year += (month <= 2);

  int32_t year_offset = year - 2000;
  if (year_offset < 0) {
    year_offset = 0;
  }

  out.year = static_cast<uint8_t>(year_offset);
  out.month = static_cast<uint8_t>(month);
  out.day = static_cast<uint8_t>(day);
  return out;
}

RadioDetails UITask::getRadioDetails() {
  RadioDetails details{};
  if (_node_prefs) {
    details.frequency = _node_prefs->freq;
    details.spreading_factor = _node_prefs->sf;
    details.bandwidth = _node_prefs->bw;
    details.coding_factor = _node_prefs->cr;
    details.transmit_power_dbm = _node_prefs->tx_power_dbm;
  }
  details.noise_floor_dbm = radio_driver.getNoiseFloor();
  details.last_rssi_dbm = radio_driver.getLastRSSI();
  details.last_snr_db = radio_driver.getLastSNR();
  details.packets_sent = radio_driver.getPacketsSent();
  details.packets_received = radio_driver.getPacketsRecv();
  return details;
}

void UITask::resetRadioStats() {
  radio_driver.resetStats();
}

uint8_t UITask::getMessages(uint8_t offset, uint8_t count, MessageEntry* out) {
  return _message_buffer.getMessages(offset, count, out);
}

void UITask::markMessageRead(uint8_t offset) {
  _message_buffer.markRead(offset);
}

const char* UITask::getFirmwareVersion() {
  return FIRMWARE_VERSION;
}

float UITask::getBatteryPercent() {
  if (millis() > _next_batt_check) {
    auto mv = getBattMilliVolts() / 1000.0;
    MESH_DEBUG_PRINTLN("Batt %f", mv); 
    auto pct = (mv - 3.3) / .9; // 3.3-4.2
    _batt_percent = pct > 1 ? 1 : pct < 0 ? 0 : pct;
    _next_batt_check = millis() + 8000;
  }

  return _batt_percent;
}

const char* UITask::getNodeName() {
  return the_mesh.getNodeName();
}
