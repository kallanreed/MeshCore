#include "ui_task.h"

#include "../MyMesh.h"
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
  renderAfter(delay_ms);
  _display->endFrame();
}

void UITask::setCurrent(UIScreen* screen) {
  _curr = screen;
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

// Public functions ---
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
  _alert_expiry = 0;

  _splash = new SplashScreen(this);
  _home = new HomeScreen(this);
  setCurrent(_splash);
}

// --- AbstractUITask ---
void UITask::msgRead(int msgcount) {}

void UITask::newMsg(
  uint8_t path_len,
  const char* from_name,
  const char* text,
  int msgcount) {}

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
  if (kb && wakeScreen()) {
    MESH_DEBUG_PRINTLN("%02x", kb);
    if (!_curr->handleInput(kb)) {
      if (kb == 'Q')
        shutdown();
      else if (kb == 'G')
        toggleGPS();
      else if (kb == 'B')
        toggleBuzzer();
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
uint32_t UITask::getBlePin() {
  return the_mesh.getBLEPin();
}

uint32_t UITask::getUptimeMin() {
  auto uptime_millis = millis() - _ui_started_at;
  return uptime_millis / 1000 / 60;
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

void UITask::toggleGPS() {
  if (!_sensors)
    return;

  // TODO: Clean up.
  int num = _sensors->getNumSettings();
  for (int i = 0; i < num; i++) {
    if (strcmp(_sensors->getSettingName(i), "gps") == 0) {
      MESH_DEBUG_PRINT("UITask::toggleGPS> ");
      if (strcmp(_sensors->getSettingValue(i), "1") == 0) {
        _sensors->setSettingValue("gps", "0");
        _node_prefs->gps_enabled = 0;
        notify(UIEventType::ack);
        MESH_DEBUG_PRINTLN("Disabled GPS");
      } else {
        _sensors->setSettingValue("gps", "1");
        _node_prefs->gps_enabled = 1;
        notify(UIEventType::ack);
        MESH_DEBUG_PRINTLN("Enabled GPS");
      }
      the_mesh.savePrefs();
      //showAlert(_node_prefs->gps_enabled ? "GPS: Enabled" : "GPS: Disabled", 800);
      //_next_refresh = 0;
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
  uint32_t seconds = epoch;
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

// #include <helpers/TxtDataHelpers.h>
// #include "../MyMesh.h"
// #include "target.h"
// #include "hardware.h"
// #ifdef WIFI_SSID
//   #include <WiFi.h>
// #endif

// #if UI_QUICK_MSG
// #include "QuickMsg.h"
// #endif

// #ifndef AUTO_OFF_MILLIS
//   #define AUTO_OFF_MILLIS    15000  // 15 seconds
// #endif
// #define BOOT_SCREEN_MILLIS   3000   // 3 seconds

// #ifdef PIN_STATUS_LED
// #define LED_ON_MILLIS     20
// #define LED_ON_MSG_MILLIS 200
// #define LED_CYCLE_MILLIS  4000
// #endif

// #define LONG_PRESS_MILLIS   1200

// #ifndef UI_RECENT_LIST_SIZE
//   #define UI_RECENT_LIST_SIZE 4
// #endif

// #include "icons.h"

// void UITask::begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs) {
//   _display = display;
//   _sensors = sensors;
//   _auto_off = millis() + AUTO_OFF_MILLIS;
//   _keyboard.begin();

// #if defined(PIN_USER_BTN)
//   user_btn.begin();
// #endif
// #if defined(PIN_USER_BTN_ANA)
//   analog_btn.begin();
// #endif

//   _node_prefs = node_prefs;

// #if ENV_INCLUDE_GPS == 1
//   // Apply GPS preferences from stored prefs
//   if (_sensors != NULL && _node_prefs != NULL) {
//     _sensors->setSettingValue("gps", _node_prefs->gps_enabled ? "1" : "0");
//     if (_node_prefs->gps_interval > 0) {
//       char interval_str[12];  // Max: 24 hours = 86400 seconds (5 digits + null)
//       sprintf(interval_str, "%u", _node_prefs->gps_interval);
//       _sensors->setSettingValue("gps_interval", interval_str);
//     }
//   }
// #endif

//   if (_display != NULL) {
//     _display->turnOn();
//   }

// #ifdef PIN_BUZZER
//   buzzer.begin();
//   buzzer.quiet(_node_prefs->buzzer_quiet);
// #endif

// #ifdef PIN_VIBRATION
//   vibration.begin();
// #endif

//   ui_started_at = millis();
//   _alert_expiry = 0;

//   splash = new SplashScreen(this);
//   home = new HomeScreen(this, &rtc_clock, sensors, node_prefs);
//   msg_preview = new MsgPreviewScreen(this, &rtc_clock);
// #if UI_QUICK_MSG
//   quick_msg = new QuickMsgScreen(this);
// #endif
//   setCurrScreen(splash);
// }

// void UITask::showAlert(const char* text, int duration_millis) {
//   strcpy(_alert, text);
//   _alert_expiry = millis() + duration_millis;
// }

// void UITask::msgRead(int msgcount) {
//   _msgcount = msgcount;
//   if (msgcount == 0) {
//     gotoHomeScreen();
//   }
// }

// void UITask::newMsg(uint8_t path_len, const char* from_name, const char* text, int msgcount) {
//   _msgcount = msgcount;

//   ((MsgPreviewScreen *) msg_preview)->addPreview(path_len, from_name, text);
//   setCurrScreen(msg_preview);

//   if (_display != NULL) {
//     if (!_display->isOn() && !hasConnection()) {
//       _display->turnOn();
//     }
//     if (_display->isOn()) {
//     _auto_off = millis() + AUTO_OFF_MILLIS;  // extend the auto-off timer
//     _next_refresh = 100;  // trigger refresh
//     }
//   }
// }

// void UITask::userLedHandler() {
// #ifdef PIN_STATUS_LED
//   int cur_time = millis();
//   if (cur_time > next_led_change) {
//     if (led_state == 0) {
//       led_state = 1;
//       if (_msgcount > 0) {
//         last_led_increment = LED_ON_MSG_MILLIS;
//       } else {
//         last_led_increment = LED_ON_MILLIS;
//       }
//       next_led_change = cur_time + last_led_increment;
//     } else {
//       led_state = 0;
//       next_led_change = cur_time + LED_CYCLE_MILLIS - last_led_increment;
//     }
//     digitalWrite(PIN_STATUS_LED, led_state == LED_STATE_ON);
//   }
// #endif
// }

// void UITask::setCurrScreen(UIScreen* c) {
//   curr = c;
//   _next_refresh = 100;
// }

// /*
//   hardware-agnostic pre-shutdown activity should be done here
// */
// void UITask::shutdown(bool restart) {

//   #ifdef PIN_BUZZER
//   /* note: we have a choice here -
//      we can do a blocking buzzer.loop() with non-deterministic consequences
//      or we can set a flag and delay the shutdown for a couple of seconds
//      while a non-blocking buzzer.loop() plays out in UITask::loop()
//   */
//   buzzer.shutdown();
//   uint32_t buzzer_timer = millis(); // fail-safe shutdown
//   while (buzzer.isPlaying() && (millis() - 2500) < buzzer_timer)
//     buzzer.loop();

//   #endif // PIN_BUZZER

//   if (restart) {
//     _board->reboot();
//   } else {
//     _display->turnOff();
//     radio_driver.powerOff();
//     _board->powerOff();
//   }
// }

// bool UITask::isButtonPressed() const {
// #ifdef PIN_USER_BTN
//   return user_btn.isPressed();
// #else
//   return false;
// #endif
// }

// void UITask::loop() {
//   uint8_t kb = _keyboard.readKeyboard();
//   if (kb != 0) {
//     MESH_DEBUG_PRINTLN("KB: 0x%02X", kb);
//     switch (kb) {
//       // TODO: Key mapping in keyboard class?
//       // TODO: Support long press and all that?
//       case KEY_UP:
//       case KEY_DOWN:
//       case KEY_LEFT:
//       case KEY_RIGHT:
//       case KEY_ENTER:
//         handleSingleClick(kb);
//         break;
//     }
//   }
// #if UI_HAS_JOYSTICK
//   int ev = user_btn.check();
//   if (ev == BUTTON_EVENT_CLICK) {
//     handleSingleClick(KEY_ENTER);
//   } else if (ev == BUTTON_EVENT_LONG_PRESS) {
//     handleLongPress(KEY_ENTER);  // REVISIT: could be mapped to different key code
//   }
//   ev = joystick_left.check();
//   if (ev == BUTTON_EVENT_CLICK) {
//     handleSingleClick(KEY_LEFT);
//   } else if (ev == BUTTON_EVENT_LONG_PRESS) {
//     handleLongPress(KEY_LEFT);
//   }
//   ev = joystick_right.check();
//   if (ev == BUTTON_EVENT_CLICK) {
//     handleSingleClick(KEY_RIGHT);
//   } else if (ev == BUTTON_EVENT_LONG_PRESS) {
//     handleLongPress(KEY_RIGHT);
//   }
//   ev = back_btn.check();
//   if (ev == BUTTON_EVENT_TRIPLE_CLICK) {
//     handleTripleClick(KEY_SELECT);
//   }
// #elif defined(PIN_USER_BTN)
//   int ev = user_btn.check();
//   if (ev == BUTTON_EVENT_CLICK) {
//     handleSingleClick(KEY_NEXT);
//   } else if (ev == BUTTON_EVENT_LONG_PRESS) {
//     handleLongPress(KEY_ENTER);
//   } else if (ev == BUTTON_EVENT_DOUBLE_CLICK) {
//     handleDoubleClick(KEY_PREV);
//   } else if (ev == BUTTON_EVENT_TRIPLE_CLICK) {
//     handleTripleClick(KEY_SELECT);
//   }
// #endif
// #if defined(PIN_USER_BTN_ANA)
//   if (abs(millis() - _analogue_pin_read_millis) > 10) {
//     ev = analog_btn.check();
//     if (ev == BUTTON_EVENT_CLICK) {
//       handleSingleClick(KEY_NEXT);
//     } else if (ev == BUTTON_EVENT_LONG_PRESS) {
//       handleLongPress(KEY_ENTER);
//     } else if (ev == BUTTON_EVENT_DOUBLE_CLICK) {
//       handleDoubleClick(KEY_PREV);
//     } else if (ev == BUTTON_EVENT_TRIPLE_CLICK) {
//       handleTripleClick(KEY_SELECT);
//     }
//     _analogue_pin_read_millis = millis();
//   }
// #endif
// #if defined(BACKLIGHT_BTN)
//   if (millis() > next_backlight_btn_check) {
//     bool touch_state = digitalRead(PIN_BUTTON2);
// #if defined(DISP_BACKLIGHT)
//     digitalWrite(DISP_BACKLIGHT, !touch_state);
// #elif defined(EXP_PIN_BACKLIGHT)
//     expander.digitalWrite(EXP_PIN_BACKLIGHT, !touch_state);
// #endif
//     next_backlight_btn_check = millis() + 300;
//   }
// #endif

//   userLedHandler();

// #ifdef PIN_BUZZER
//   if (buzzer.isPlaying())  buzzer.loop();
// #endif

//   if (curr) curr->poll();

//   if (_display != NULL && _display->isOn()) {
//     if (millis() >= _next_refresh && curr) {
//       _display->startFrame();
//       int delay_millis = curr->render(*_display);
//       if (millis() < _alert_expiry) {  // render alert popup
//         _display->setTextSize(1);
//         int y = _display->height() / 3;
//         int p = _display->height() / 32;
//         _display->setColor(DisplayDriver::DARK);
//         _display->fillRect(p, y, _display->width() - p*2, y);
//         _display->setColor(DisplayDriver::LIGHT);  // draw box border
//         _display->drawRect(p, y, _display->width() - p*2, y);
//         _display->drawTextCentered(_display->width() / 2, y + p*3, _alert);
//         _next_refresh = _alert_expiry;   // will need refresh when alert is dismissed
//       } else {
//         _next_refresh = millis() + delay_millis;
//       }
//       _display->endFrame();
//     }
// #if AUTO_OFF_MILLIS > 0
//     if (millis() > _auto_off) {
//       _display->turnOff();
//     }
// #endif
//   }

// #ifdef PIN_VIBRATION
//   vibration.loop();
// #endif

// #ifdef AUTO_SHUTDOWN_MILLIVOLTS
//   if (millis() > next_batt_chck) {
//     uint16_t milliVolts = getBattMilliVolts();
//     if (milliVolts > 0 && milliVolts < AUTO_SHUTDOWN_MILLIVOLTS) {

//       // show low battery shutdown alert
//       // we should only do this for eink displays, which will persist after power loss
//       #if defined(THINKNODE_M1) || defined(LILYGO_TECHO)
//       if (_display != NULL) {
//         _display->startFrame();
//         _display->setTextSize(2);
//         _display->setColor(DisplayDriver::RED);
//         _display->drawTextCentered(_display->width() / 2, 20, "Low Battery.");
//         _display->drawTextCentered(_display->width() / 2, 40, "Shutting Down!");
//         _display->endFrame();
//       }
//       #endif

//       shutdown();

//     }
//     next_batt_chck = millis() + 8000;
//   }
// #endif
// }

// bool UITask::checkDisplayOn() {
//   // ensures that the display is on and the timer is reset
//   // returns false if the display was previously off
//   bool display_on = false;
//   if (_display != NULL) {
//     if (!_display->isOn()) {
//       _display->turnOn();  // turn display on
//     } else {
//       display_on = true;
//     }
//     _auto_off = millis() + AUTO_OFF_MILLIS;  // extend auto-off timer
//     _next_refresh = 0;  // trigger refresh
//   }
//   return display_on;
// }

// void UITask::handleLongPress(char c) {
//   if (millis() - ui_started_at < 8000) {  // long press in first 8 seconds since startup -> CLI/rescue
//     the_mesh.enterCLIRescue();
//   } else {
//     MESH_DEBUG_PRINTLN("UITask: long press triggered");
//     uiHandleKey(c);
//   }
// }

// void UITask::handleSingleClick(char c) {
//   MESH_DEBUG_PRINTLN("UITask: single click triggered");
//   uiHandleKey(c);
// }

// void UITask::handleDoubleClick(char c) {
//   MESH_DEBUG_PRINTLN("UITask: double click triggered");
//   uiHandleKey(c);
// }

// void UITask::handleTripleClick(char c) {
//   MESH_DEBUG_PRINTLN("UITask: triple click triggered");
//   if (!uiHandleKey(c)) toggleBuzzer();
// }

// bool UITask::uiHandleKey(char c) {
//   bool handled = false;
//   bool display_on = checkDisplayOn();

//   if (c != 0 && display_on && curr) {
//     handled = curr->handleInput(c);
//     _auto_off = millis() + AUTO_OFF_MILLIS; // extend auto-off timer
//     _next_refresh = 100;  // trigger refresh
//   }
//   return handled;
// }

// bool UITask::getGPSState() {
//   if (_sensors != NULL) {
//     int num = _sensors->getNumSettings();
//     for (int i = 0; i < num; i++) {
//       if (strcmp(_sensors->getSettingName(i), "gps") == 0) {
//         return !strcmp(_sensors->getSettingValue(i), "1");
//       }
//     }
//   }
//   return false;
// }
