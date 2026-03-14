#include "ui_task.h"

#include <stdio.h>
#include <string.h>

#include "../MyMesh.h"
#include <helpers/AdvertDataHelpers.h>
#include <helpers/ChannelDetails.h>
#include <helpers/TxtDataHelpers.h>
#include "screens.h"
#include "keys.h"
#include "utf8.h"

constexpr uint32_t auto_off_ms = 15 * 1000;

// --- Helpers  ---
static bool getChatContactByIndex(uint8_t contact_index, ContactInfo& out) {
  if (!the_mesh.getContactByIdx(contact_index, out))
    return false;
  return out.type == ADV_TYPE_CHAT;
}

static bool getContactPrefixByIndex(uint8_t contact_index, uint8_t* out_prefix) {
  if (!out_prefix)
    return false;
  ContactInfo contact{};
  if (!getChatContactByIndex(contact_index, contact))
    return false;
  memcpy(out_prefix, contact.id.pub_key, kContactPrefixSize);
  return true;
}

static bool handleKey(UITask* ui, char key) {
  if (isKey(key, KeyCode::FN_H)) {
    ui->gotoHome();
    return true;
  }
  if (isKey(key, KeyCode::FN_I)) {
    ui->toggleScreenInvert();
    return true;
  }
  return false;
}

static void formatOutgoingMessage(char* out, size_t out_size, const char* text) {
  if (!out || out_size == 0)
    return;
  if (!text) {
    out[0] = 0;
    return;
  }

  char prefixed[kMessageTextSize];
  snprintf(prefixed, sizeof(prefixed), "> %s", text);
  translateUTF8ToBlocks(out, prefixed, out_size);
}

static void formatAckContactLabel(char* out, size_t out_size, const ContactInfo& contact) {
  if (!out || out_size == 0)
    return;

  if (contact.name[0]) {
    StrHelper::strncpy(out, contact.name, out_size);
    return;
  }

  snprintf(
    out,
    out_size,
    "ID:%02X%02X%02X",
    contact.id.pub_key[0],
    contact.id.pub_key[1],
    contact.id.pub_key[2]);
}


// --- Private functions ---
void UITask::dispatchRender() {
  if (!_display || !_display->isOn())
    return;

  if (millis() < _next_render || !_curr)
    return;

  _display->startFrame();
  auto delay_ms = _curr->render(*_display);
  if (_prompt.isActive()) {
    _prompt.render(*_display);
  }
  renderDmAckBadge();
  if (_invert_screen) {
    _display->setColor(DisplayDriver::INVERSE);
    _display->fillRect(0, 0, 240, 135);
    _display->setColor(DisplayDriver::LIGHT);
  }
  renderAfter(delay_ms);
  _display->endFrame();
}

void UITask::updateBme680History() {
  uint32_t now = millis();
  if (!_bme680_history.needsSample(now))
    return;

  Bme680Data data = readBme680Data();
  _bme680_history.tick(now, data);
}

Bme680Data UITask::readBme680Data() {
  Bme680Data data{};
  if (_sensors) {
    _sensors_lpp.reset();
    if (_sensors->querySensors(TELEM_PERM_ENVIRONMENT, _sensors_lpp)) {
      decodeBme680FromLpp(_sensors_lpp.getBuffer(), _sensors_lpp.getSize(), data);
    }
  }
  return data;
}

void UITask::renderDmAckBadge() {
  if (!_display || _dm_ack_label[0] == 0)
    return;

  uint32_t now = millis();
  if (now >= _dm_ack_expires_at) {
    _dm_ack_label[0] = 0;
    _dm_ack_trip_time_ms = 0;
    return;
  }

  char badge[40];
  if (_dm_ack_trip_time_ms > 0) {
    snprintf(badge, sizeof(badge), "DM ACK %s %lums", _dm_ack_label, _dm_ack_trip_time_ms);
  } else {
    snprintf(badge, sizeof(badge), "DM ACK %s", _dm_ack_label);
  }

  _display->setTextSize(1);
  int badge_w = _display->getTextWidth(badge) + 8;
  if (badge_w > _display->width() - 4)
    badge_w = _display->width() - 4;

  int badge_h = 12;
  int badge_x = _display->width() - badge_w - 2;
  int badge_y = 2;

  _display->setColor(DisplayDriver::LIGHT);
  _display->drawTextLeftAlign(badge_x + 4, badge_y + 2, badge);
  _display->setColor(DisplayDriver::INVERSE);
  _display->fillRect(badge_x, badge_y, badge_w, badge_h);
  _display->setColor(DisplayDriver::LIGHT);
}

void UITask::setCurrent(UIScreen* screen) {
  if (_curr != screen)
    _prev_screen = _curr;

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
  _buzzer.begin(false);
  _buzzer.quiet(_node_prefs->buzzer_quiet);

  wakeScreen();
  _ui_started_at = millis();

  _splash = new SplashScreen(this);
  _home = new HomeScreen(this);
  _msg_viewer = new MsgViewer(this);
  _thread_viewer = new ThreadScreen(this);
  _text_input = new TextInputScreen(this);
  _sensor_chart = new ChartScreen(this);
  setCurrent(_splash);
}

// --- AbstractUITask ---
void UITask::newMsg(
  uint8_t path_len,
  const char* from_name,
  const char* text,
  int msgcount,
  const UIMessageMeta& meta) {
  char sender[kMessageSenderSize];
  char message[kMessageTextSize];
  const char* safe_sender = from_name ? from_name : "";
  const char* safe_message = text ? text : "";

  translateUTF8ToBlocks(sender, safe_sender, sizeof(sender));
  translateUTF8ToBlocks(message, safe_message, sizeof(message));

  MessageKind kind = MessageKind::unknown;
  if (meta.kind == UIMessageKind::contact) {
    kind = MessageKind::contact;
  } else if (meta.kind == UIMessageKind::channel) {
    kind = MessageKind::channel;
  }

  _message_buffer.addMessage(
    millis(),
    sender,
    message,
    kind,
    meta.contact_prefix,
    meta.channel_index,
    MessageDirection::incoming);
  renderAfter(0);
}

void UITask::onDirectMessageAck(const ContactInfo& contact, uint32_t trip_time_ms) {
  formatAckContactLabel(_dm_ack_label, sizeof(_dm_ack_label), contact);
  _dm_ack_trip_time_ms = trip_time_ms;
  _dm_ack_expires_at = millis() + 5000;
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
      // Screen was off; activate only and swallow the key.
      _curr->activate();
    } else if (handleKey(this, kb)) {
      // Key handled by global UI shortcut.
    } else if (_prompt.isActive()) {
      _prompt.handleInput(kb);
    } else {
      _curr->handleInput(kb);
    }
  }

  updateBme680History();

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

void UITask::markAllMessagesRead() {
  _message_buffer.markAllRead();
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
  if (count == 0)
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
  auto success = the_mesh.sendGroupMessage(now, details.channel, name, text, len);
  if (success) {
    char message[kMessageTextSize];
    formatOutgoingMessage(message, sizeof(message), text);
    _message_buffer.addMessage(
      millis(),
      "You",
      message,
      MessageKind::channel,
      nullptr,
      channel_index,
      MessageDirection::outgoing);
    renderAfter(0);
  }
  return success;
}

uint8_t UITask::getChannelIndexes(uint8_t* indexes, uint8_t max) {
  if (!indexes || max == 0)
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

    indexes[count++] = i;
  }

  return count;
}

const char* UITask::getChannelName(uint8_t channel_index) {
  static ChannelDetails details;
  if (!the_mesh.getChannel(channel_index, details))
    return nullptr;

  return details.name;
}

uint8_t UITask::getMsgCountForChannel(uint8_t channel_index) {
  return _message_buffer.getCountForChannel(channel_index);
}

uint8_t UITask::getUnreadCountForChannel(uint8_t channel_index) {
  return _message_buffer.getUnreadCountForChannel(channel_index);
}

uint8_t UITask::getMessagesForChannel(
  uint8_t channel_index,
  uint8_t offset,
  uint8_t count,
  MessageEntry* out) {
  return _message_buffer.getMessagesForChannel(offset, count, out, channel_index);
}

void UITask::markMessagesReadForChannel(uint8_t channel_index) {
  _message_buffer.markAllReadForChannel(channel_index);
}

void UITask::gotoChannelThread(uint8_t channel_index) {
  static_cast<ThreadScreen*>(_thread_viewer)->setChannel(channel_index);
  setCurrent(_thread_viewer);
}

uint8_t UITask::getContactIndexes(uint8_t* indexes, uint8_t max) {
  if (!indexes || max == 0)
    return 0;

  uint8_t count = 0;
  auto total = the_mesh.getNumContacts();
  for (uint32_t i = 0; i < static_cast<uint32_t>(total) && count < max; i++) {
    ContactInfo contact{};
    if (!the_mesh.getContactByIdx(i, contact))
      continue;
    if (contact.type != ADV_TYPE_CHAT)
      continue;
    indexes[count++] = static_cast<uint8_t>(i);
  }
  return count;
}

const char* UITask::getContactName(uint8_t contact_index) {
  static char name_buf[40];
  ContactInfo contact{};
  if (!getChatContactByIndex(contact_index, contact))
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

bool UITask::sendContactMessage(uint8_t contact_index, const char* text) {
  if (!text)
    return false;

  auto len = strlen(text);
  if (len == 0)
    return false;

  ContactInfo contact{};
  if (!getChatContactByIndex(contact_index, contact))
    return false;

  auto now = the_mesh.getRTCClock()->getCurrentTime();
  uint32_t expected_ack = 0;
  uint32_t est_timeout = 0;
  auto result = the_mesh.sendMessage(contact, now, 0, text, expected_ack, est_timeout);
  auto success = result != MSG_SEND_FAILED;
  if (success) {
    char message[kMessageTextSize];
    formatOutgoingMessage(message, sizeof(message), text);
    _message_buffer.addMessage(
      millis(),
      "You",
      message,
      MessageKind::contact,
      contact.id.pub_key,
      0xFF,
      MessageDirection::outgoing);
    renderAfter(0);
  }
  return success;
}

uint8_t UITask::getMsgCountForContact(uint8_t contact_index) {
  uint8_t prefix[kContactPrefixSize] = {};
  if (!getContactPrefixByIndex(contact_index, prefix))
    return 0;
  return _message_buffer.getCountForContact(prefix);
}

uint8_t UITask::getUnreadCountForContact(uint8_t contact_index) {
  uint8_t prefix[kContactPrefixSize] = {};
  if (!getContactPrefixByIndex(contact_index, prefix))
    return 0;
  return _message_buffer.getUnreadCountForContact(prefix);
}

uint8_t UITask::getMessagesForContact(
  uint8_t contact_index,
  uint8_t offset,
  uint8_t count,
  MessageEntry* out) {
  uint8_t prefix[kContactPrefixSize] = {};
  if (!getContactPrefixByIndex(contact_index, prefix))
    return 0;
  return _message_buffer.getMessagesForContact(offset, count, out, prefix);
}

void UITask::markMessagesReadForContact(uint8_t contact_index) {
  uint8_t prefix[kContactPrefixSize] = {};
  if (!getContactPrefixByIndex(contact_index, prefix))
    return;
  _message_buffer.markAllReadForContact(prefix);
}

void UITask::gotoContactThread(uint8_t contact_index) {
  static_cast<ThreadScreen*>(_thread_viewer)->setContact(contact_index);
  setCurrent(_thread_viewer);
}

uint32_t UITask::getBlePin() {
  return the_mesh.getBLEPin();
}

uint32_t UITask::getUptimeMin() {
  auto uptime_millis = millis() - _ui_started_at;
  return uptime_millis / 1000 / 60;
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

bool UITask::isCampModeEnabled() {
  return _node_prefs->client_repeat != 0;
}

void UITask::toggleCampMode() {
  _node_prefs->client_repeat = _node_prefs->client_repeat ? 0 : 1;
  the_mesh.savePrefs();
}

void UITask::gotoHome() {
  setCurrent(_home);
  _prev_screen = nullptr;
}

void UITask::gotoPrevious() {
  if (_prev_screen) {
    setCurrent(_prev_screen);
  } else {
    gotoHome();
  }
}

void UITask::gotoMsgViewer(const MessageEntry& message, MessageScope scope) {
  static_cast<MsgViewer*>(_msg_viewer)->setMessage(message, scope);
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
}

void UITask::toggleScreenInvert() {
  _invert_screen = !_invert_screen;
  renderAfter(0);
}

bool UITask::sendAdvert(bool flood) {
  notify(UIEventType::ack);
  return the_mesh.advert(flood);
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

void UITask::setTzOffset(int8_t offset_hours) {
  _node_prefs->tz_offset = constrain(offset_hours, -12, 14);
  the_mesh.savePrefs();
}

Position UITask::getPosition() {
  Position p{};
  LocationProvider* nmea = sensors.getLocationProvider();

  if (nmea) {
    p.has_fix = nmea->isValid();
    if (p.has_fix) {
      p.latitude = nmea->getLatitude() / 1000000.0;
      p.longitude = nmea->getLongitude() / 1000000.0;
      p.elevation = nmea->getAltitude() / 1000.0; // m
      p.satellites = nmea->satellitesCount();
    }
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
  out.tz_offset = _node_prefs ? _node_prefs->tz_offset : 0;
  int32_t offset = ((int32_t)out.tz_offset) * 60 * 60;
  out.is_valid = _rtc->isValid();

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

Bme680Data UITask::getBme680Data() {
  return _bme680_history.latest();
}

uint8_t UITask::getBme680History(Bme680Metric metric, float* out, uint8_t max) {
  return _bme680_history.get(metric, out, max);
}

uint8_t UITask::getBme680HistoryIntervalMin() {
  return _bme680_history.getIntervalMinutes();
}

void UITask::gotoChart(ChartConfig* config) {
  setCurrent(_sensor_chart);
  static_cast<ChartScreen*>(_sensor_chart)->setConfig(config);
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

void UITask::markMessageReadById(uint32_t message_id) {
  _message_buffer.markReadById(message_id);
}

bool UITask::getPreviousMessage(MessageScope scope, const MessageEntry& current, MessageEntry* out) {
  return _message_buffer.getPreviousMessage(scope, current, out);
}

bool UITask::getNextMessage(MessageScope scope, const MessageEntry& current, MessageEntry* out) {
  return _message_buffer.getNextMessage(scope, current, out);
}

uint8_t UITask::getRecentAdverts(RecentAdvertEntry* out, uint8_t max) {
  if (!out || max == 0)
    return 0;

  if (max > kRecentAdvertMax)
    max = kRecentAdvertMax;

  AdvertPath recent[kRecentAdvertMax] = {};
  auto total = the_mesh.getRecentlyHeard(recent, max);
  uint8_t count = 0;

  for (int i = 0; i < total && count < max; i++) {
    if (recent[i].type != ADV_TYPE_CHAT)
      continue;
    if (recent[i].name[0] == 0)
      continue;

    auto& entry = out[count++];
    StrHelper::strncpy(entry.name, recent[i].name, sizeof(entry.name));
    entry.recv_timestamp = recent[i].recv_timestamp;
    memcpy(entry.pub_key, recent[i].pub_key, sizeof(entry.pub_key));
  }

  return count;
}

bool UITask::hasContact(const uint8_t* pub_key) {
  if (!pub_key)
    return false;

  return the_mesh.lookupContactByPubKey(pub_key, PUB_KEY_SIZE) != nullptr;
}

bool UITask::addRecentAdvertContact(const RecentAdvertEntry& advert) {
  if (advert.pub_key[0] == 0)
    return false;

  return the_mesh.addChatContactFromRecent(advert.pub_key, advert.name);
}

uint32_t UITask::getRtcSeconds() {
  return the_mesh.getRTCClock()->getCurrentTime();
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
