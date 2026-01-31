#include "screens.h"
#include "ui_task.h"
#include "../MyMesh.h"

// --- SplashScreen ---
SplashScreen::SplashScreen(UIViewModel* model) : _model(model) {
  // strip off dash and commit hash by changing dash to null terminator
  // e.g: v1.2.3-abcdef -> v1.2.3
  const char *ver = FIRMWARE_VERSION;
  const char *dash = strchr(ver, '-');

  int len = dash ? dash - ver : strlen(ver);
  if (len >= sizeof(_version_info)) len = sizeof(_version_info) - 1;
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

// --- Page Instances ---
extern UITask ui_task;
static UIViewModel* view_model = &ui_task;
static HomePage homePage = HomePage(view_model);
static GpsPage gpsPage = GpsPage(view_model);
static ClockPage clockPage = ClockPage(view_model);
static PowerPage powerPage = PowerPage(view_model);

// --- HomeScreen ---
HomeScreen::HomeScreen(UIViewModel* model) : _model(model) {
  _pages[0] = &homePage;
  _pages[1] = &homePage;
  _pages[2] = &homePage;
  _pages[3] = &homePage;
  _pages[4] = &homePage;
  _pages[5] = &gpsPage;
  _pages[6] = &clockPage;
  _pages[7] = &powerPage;
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

  return 1000;
}

bool HomeScreen::handleInput(char c) {
  bool handled = false;

  // TODO: allow page first dibs?

  if (c == KEY_LEFT) {
    _page = (_pages.size() + _page - 1) % _pages.size();
    handled = true;
  } else if (c == KEY_RIGHT) {
    _page = (_page + 1) % _pages.size();
    handled = true;
  } else if (c == KEY_ENTER) {
    current()->activate();
    handled = true;
  }

  if (handled)
    _model->renderAfter(0);

  return handled;
}

void HomeScreen::poll() {
}

// class HomeScreen : public UIScreen {
//   enum HomePage {
//     FIRST,
//     RECENT,
//     RADIO,
//     BLUETOOTH,
//     ADVERT,
// #if ENV_INCLUDE_GPS == 1
//     GPS,
// #endif
// #if UI_SENSORS_PAGE == 1
//     SENSORS,
// #endif
// #if UI_QUICK_MSG
//     QUICK_MSG,
// #endif
//     SHUTDOWN,
//     Count    // keep as last
//   };

//   UITask* _task;
//   mesh::RTCClock* _rtc;
//   SensorManager* _sensors;
//   NodePrefs* _node_prefs;
//   uint8_t _page;
//   bool _shutdown_init;
//   AdvertPath recent[UI_RECENT_LIST_SIZE];

//   void renderBatteryIndicator(DisplayDriver& display, uint16_t batteryMilliVolts) {
//     // Convert millivolts to percentage
//     const int minMilliVolts = 3000; // Minimum voltage (e.g., 3.0V)
//     const int maxMilliVolts = 4200; // Maximum voltage (e.g., 4.2V)
//     int batteryPercentage = ((batteryMilliVolts - minMilliVolts) * 100) / (maxMilliVolts - minMilliVolts);
//     if (batteryPercentage < 0) batteryPercentage = 0; // Clamp to 0%
//     if (batteryPercentage > 100) batteryPercentage = 100; // Clamp to 100%

//     // battery icon
//     int iconWidth = 24;
//     int iconHeight = 10;
//     int iconX = display.width() - iconWidth - 5; // Position the icon near the top-right corner
//     int iconY = 0;
//     display.setColor(DisplayDriver::GREEN);

//     // battery outline
//     display.drawRect(iconX, iconY, iconWidth, iconHeight);

//     // battery "cap"
//     display.fillRect(iconX + iconWidth, iconY + (iconHeight / 4), 3, iconHeight / 2);

//     // fill the battery based on the percentage
//     int fillWidth = (batteryPercentage * (iconWidth - 4)) / 100;
//     display.fillRect(iconX + 2, iconY + 2, fillWidth, iconHeight - 4);
//   }

//   CayenneLPP sensors_lpp;
//   int sensors_nb = 0;
//   bool sensors_scroll = false;
//   int sensors_scroll_offset = 0;
//   int next_sensors_refresh = 0;

//   void refresh_sensors() {
//     if (millis() > next_sensors_refresh) {
//       sensors_lpp.reset();
//       sensors_nb = 0;
//       sensors_lpp.addVoltage(TELEM_CHANNEL_SELF, (float)board.getBattMilliVolts() / 1000.0f);
//       sensors.querySensors(0xFF, sensors_lpp);
//       LPPReader reader (sensors_lpp.getBuffer(), sensors_lpp.getSize());
//       uint8_t channel, type;
//       while(reader.readHeader(channel, type)) {
//         reader.skipData(type);
//         sensors_nb ++;
//       }
//       sensors_scroll = sensors_nb > UI_RECENT_LIST_SIZE;
// #if AUTO_OFF_MILLIS > 0
//       next_sensors_refresh = millis() + 5000; // refresh sensor values every 5 sec
// #else
//       next_sensors_refresh = millis() + 60000; // refresh sensor values every 1 min
// #endif
//     }
//   }

// public:
//   HomeScreen(UITask* task, mesh::RTCClock* rtc, SensorManager* sensors, NodePrefs* node_prefs)
//      : _task(task), _rtc(rtc), _sensors(sensors), _node_prefs(node_prefs), _page(0),
//        _shutdown_init(false), sensors_lpp(200) {  }

//   void poll() override {
//     if (_shutdown_init && !_task->isButtonPressed()) {  // must wait for USR button to be released
//       _task->shutdown();
//     }
//   }

//   int render(DisplayDriver& display) override {
//     char tmp[80];
//     // node name
//     display.setTextSize(1);
//     display.setColor(DisplayDriver::GREEN);
//     char filtered_name[sizeof(_node_prefs->node_name)];
//     display.translateUTF8ToBlocks(filtered_name, _node_prefs->node_name, sizeof(filtered_name));
//     display.setCursor(0, 0);
//     display.print(filtered_name);

//     // battery voltage
//     renderBatteryIndicator(display, _task->getBattMilliVolts());

//     // curr page indicator
//     int y = 14;
//     int x = display.width() / 2 - 5 * (HomePage::Count-1);
//     for (uint8_t i = 0; i < HomePage::Count; i++, x += 10) {
//       if (i == _page) {
//         display.fillRect(x-1, y-1, 3, 3);
//       } else {
//         display.fillRect(x, y, 1, 1);
//       }
//     }

//     if (_page == HomePage::FIRST) {
//       display.setColor(DisplayDriver::YELLOW);
//       display.setTextSize(2);
//       sprintf(tmp, "MSG: %d", _task->getMsgCount());
//       display.drawTextCentered(display.width() / 2, 20, tmp);

//       #ifdef WIFI_SSID
//         IPAddress ip = WiFi.localIP();
//         snprintf(tmp, sizeof(tmp), "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
//         display.setTextSize(1);
//         display.drawTextCentered(display.width() / 2, 54, tmp);
//       #endif
//       if (_task->hasConnection()) {
//         display.setColor(DisplayDriver::GREEN);
//         display.setTextSize(1);
//         display.drawTextCentered(display.width() / 2, 43, "< Connected >");

//       } else if (the_mesh.getBLEPin() != 0) { // BT pin
//         display.setColor(DisplayDriver::RED);
//         display.setTextSize(2);
//         sprintf(tmp, "Pin:%d", the_mesh.getBLEPin());
//         display.drawTextCentered(display.width() / 2, 43, tmp);
//       }
//     } else if (_page == HomePage::RECENT) {
//       the_mesh.getRecentlyHeard(recent, UI_RECENT_LIST_SIZE);
//       display.setColor(DisplayDriver::GREEN);
//       int y = 20;
//       for (int i = 0; i < UI_RECENT_LIST_SIZE; i++, y += 11) {
//         auto a = &recent[i];
//         if (a->name[0] == 0) continue;  // empty slot
//         int secs = _rtc->getCurrentTime() - a->recv_timestamp;
//         if (secs < 60) {
//           sprintf(tmp, "%ds", secs);
//         } else if (secs < 60*60) {
//           sprintf(tmp, "%dm", secs / 60);
//         } else {
//           sprintf(tmp, "%dh", secs / (60*60));
//         }

//         int timestamp_width = display.getTextWidth(tmp);
//         int max_name_width = display.width() - timestamp_width - 1;

//         char filtered_recent_name[sizeof(a->name)];
//         display.translateUTF8ToBlocks(filtered_recent_name, a->name, sizeof(filtered_recent_name));
//         display.drawTextEllipsized(0, y, max_name_width, filtered_recent_name);
//         display.setCursor(display.width() - timestamp_width - 1, y);
//         display.print(tmp);
//       }
//     } else if (_page == HomePage::RADIO) {
//       display.setColor(DisplayDriver::YELLOW);
//       display.setTextSize(1);
//       // freq / sf
//       display.setCursor(0, 20);
//       sprintf(tmp, "FQ: %06.3f   SF: %d", _node_prefs->freq, _node_prefs->sf);
//       display.print(tmp);

//       display.setCursor(0, 31);
//       sprintf(tmp, "BW: %03.2f     CR: %d", _node_prefs->bw, _node_prefs->cr);
//       display.print(tmp);

//       // tx power,  noise floor
//       display.setCursor(0, 42);
//       sprintf(tmp, "TX: %ddBm", _node_prefs->tx_power_dbm);
//       display.print(tmp);
//       display.setCursor(0, 53);
//       sprintf(tmp, "Noise floor: %d", radio_driver.getNoiseFloor());
//       display.print(tmp);
//     } else if (_page == HomePage::BLUETOOTH) {
//       display.setColor(DisplayDriver::GREEN);
//       display.drawXbm((display.width() - 32) / 2, 18,
//           _task->isSerialEnabled() ? bluetooth_on : bluetooth_off,
//           32, 32);
//       display.setTextSize(1);
//       display.drawTextCentered(display.width() / 2, 64 - 11, "toggle: " PRESS_LABEL);
//     } else if (_page == HomePage::ADVERT) {
//       display.setColor(DisplayDriver::GREEN);
//       display.drawXbm((display.width() - 32) / 2, 18, advert_icon, 32, 32);
//       display.drawTextCentered(display.width() / 2, 64 - 11, "advert: " PRESS_LABEL);
// #if ENV_INCLUDE_GPS == 1
//     } else if (_page == HomePage::GPS) {
//       LocationProvider* nmea = sensors.getLocationProvider();
//       char buf[50];
//       int y = 18;
//       bool gps_state = _task->getGPSState();
// #ifdef PIN_GPS_SWITCH
//       bool hw_gps_state = digitalRead(PIN_GPS_SWITCH);
//       if (gps_state != hw_gps_state) {
//         strcpy(buf, gps_state ? "gps off(hw)" : "gps off(sw)");
//       } else {
//         strcpy(buf, gps_state ? "gps on" : "gps off");
//       }
// #else
//       strcpy(buf, gps_state ? "gps on" : "gps off");
// #endif
//       display.drawTextLeftAlign(0, y, buf);
//       if (nmea == NULL) {
//         y = y + 12;
//         display.drawTextLeftAlign(0, y, "Can't access GPS");
//       } else {
//         strcpy(buf, nmea->isValid()?"fix":"no fix");
//         display.drawTextRightAlign(display.width()-1, y, buf);
//         y = y + 12;
//         display.drawTextLeftAlign(0, y, "sat");
//         sprintf(buf, "%d", nmea->satellitesCount());
//         display.drawTextRightAlign(display.width()-1, y, buf);
//         y = y + 12;
//         display.drawTextLeftAlign(0, y, "pos");
//         sprintf(buf, "%.4f %.4f",
//           nmea->getLatitude()/1000000., nmea->getLongitude()/1000000.);
//         display.drawTextRightAlign(display.width()-1, y, buf);
//         y = y + 12;
//         display.drawTextLeftAlign(0, y, "alt");
//         sprintf(buf, "%.2f", nmea->getAltitude()/1000.);
//         display.drawTextRightAlign(display.width()-1, y, buf);
//         y = y + 12;
//       }
// #endif
// #if UI_SENSORS_PAGE == 1
//     } else if (_page == HomePage::SENSORS) {
//       int y = 18;
//       refresh_sensors();
//       char buf[30];
//       char name[30];
//       LPPReader r(sensors_lpp.getBuffer(), sensors_lpp.getSize());

//       for (int i = 0; i < sensors_scroll_offset; i++) {
//         uint8_t channel, type;
//         r.readHeader(channel, type);
//         r.skipData(type);
//       }

//       for (int i = 0; i < (sensors_scroll?UI_RECENT_LIST_SIZE:sensors_nb); i++) {
//         uint8_t channel, type;
//         if (!r.readHeader(channel, type)) { // reached end, reset
//           r.reset();
//           r.readHeader(channel, type);
//         }

//         display.setCursor(0, y);
//         float v;
//         switch (type) {
//           case LPP_GPS: // GPS
//             float lat, lon, alt;
//             r.readGPS(lat, lon, alt);
//             strcpy(name, "gps"); sprintf(buf, "%.4f %.4f", lat, lon);
//             break;
//           case LPP_VOLTAGE:
//             r.readVoltage(v);
//             strcpy(name, "voltage"); sprintf(buf, "%6.2f", v);
//             break;
//           case LPP_CURRENT:
//             r.readCurrent(v);
//             strcpy(name, "current"); sprintf(buf, "%.3f", v);
//             break;
//           case LPP_TEMPERATURE:
//             r.readTemperature(v);
//             strcpy(name, "temperature"); sprintf(buf, "%.2f", v);
//             break;
//           case LPP_RELATIVE_HUMIDITY:
//             r.readRelativeHumidity(v);
//             strcpy(name, "humidity"); sprintf(buf, "%.2f", v);
//             break;
//           case LPP_BAROMETRIC_PRESSURE:
//             r.readPressure(v);
//             strcpy(name, "pressure"); sprintf(buf, "%.2f", v);
//             break;
//           case LPP_ALTITUDE:
//             r.readAltitude(v);
//             strcpy(name, "altitude"); sprintf(buf, "%.0f", v);
//             break;
//           case LPP_POWER:
//             r.readPower(v);
//             strcpy(name, "power"); sprintf(buf, "%6.2f", v);
//             break;
//           default:
//             r.skipData(type);
//             strcpy(name, "unk"); sprintf(buf, "");
//         }
//         display.setCursor(0, y);
//         display.print(name);
//         display.setCursor(
//           display.width()-display.getTextWidth(buf)-1, y
//         );
//         display.print(buf);
//         y = y + 12;
//       }
//       if (sensors_scroll) sensors_scroll_offset = (sensors_scroll_offset+1)%sensors_nb;
//       else sensors_scroll_offset = 0;
// #endif
// #if UI_QUICK_MSG
//     } else if (_page == HomePage::QUICK_MSG) {
//       display.setColor(DisplayDriver::YELLOW);
//       display.setTextSize(2);
//       display.drawTextCentered(display.width() / 2, 24, "quick msg");
//       display.setTextSize(1);
//       display.drawTextCentered(display.width() / 2, 40, "enter/exit: " PRESS_LABEL);
// #endif
//     } else if (_page == HomePage::SHUTDOWN) {
//       display.setColor(DisplayDriver::GREEN);
//       display.setTextSize(1);
//       if (_shutdown_init) {
//         display.drawTextCentered(display.width() / 2, 34, "hibernating...");
//       } else {
//         display.drawXbm((display.width() - 32) / 2, 18, power_icon, 32, 32);
//         display.drawTextCentered(display.width() / 2, 64 - 11, "hibernate: " PRESS_LABEL);
//       }
//     }
//     return 5000;   // next render after 5000 ms
//   }

//   bool handleInput(char c) override {
//     if (c == KEY_LEFT || c == KEY_PREV) {
//       _page = (_page + HomePage::Count - 1) % HomePage::Count;
//       return true;
//     }
//     if (c == KEY_NEXT || c == KEY_RIGHT) {
//       _page = (_page + 1) % HomePage::Count;
//       if (_page == HomePage::RECENT) {
//         _task->showAlert("Recent adverts", 800);
//       }
//       return true;
//     }
//     if (c == KEY_ENTER && _page == HomePage::BLUETOOTH) {
//       if (_task->isSerialEnabled()) {  // toggle Bluetooth on/off
//         _task->disableSerial();
//       } else {
//         _task->enableSerial();
//       }
//       return true;
//     }
//     if (c == KEY_ENTER && _page == HomePage::ADVERT) {
//       _task->notify(UIEventType::ack);
//       if (the_mesh.advert()) {
//         _task->showAlert("Advert sent!", 1000);
//       } else {
//         _task->showAlert("Advert failed..", 1000);
//       }
//       return true;
//     }
// #if ENV_INCLUDE_GPS == 1
//     if (c == KEY_ENTER && _page == HomePage::GPS) {
//       _task->toggleGPS();
//       return true;
//     }
// #endif
// #if UI_SENSORS_PAGE == 1
//     if (c == KEY_ENTER && _page == HomePage::SENSORS) {
//       _task->toggleGPS();
//       next_sensors_refresh=0;
//       return true;
//     }
// #endif
// #if UI_QUICK_MSG
//     if (c == KEY_ENTER && _page == HomePage::QUICK_MSG) {
//       _task->gotoQuickMsgScreen();
//       return true;
//     }
// #endif
//     if (c == KEY_ENTER && _page == HomePage::SHUTDOWN) {
//       _shutdown_init = true;  // need to wait for button to be released
//       return true;
//     }
//     return false;
//   }
// };

// class MsgPreviewScreen : public UIScreen {
//   UITask* _task;
//   mesh::RTCClock* _rtc;

//   struct MsgEntry {
//     uint32_t timestamp;
//     char origin[62];
//     char msg[78];
//   };
//   #define MAX_UNREAD_MSGS   32
//   int num_unread;
//   MsgEntry unread[MAX_UNREAD_MSGS];

// public:
//   MsgPreviewScreen(UITask* task, mesh::RTCClock* rtc) : _task(task), _rtc(rtc) { num_unread = 0; }

//   void addPreview(uint8_t path_len, const char* from_name, const char* msg) {
//     if (num_unread >= MAX_UNREAD_MSGS) return;  // full

//     auto p = &unread[num_unread++];
//     p->timestamp = _rtc->getCurrentTime();
//     if (path_len == 0xFF) {
//       sprintf(p->origin, "(D) %s:", from_name);
//     } else {
//       sprintf(p->origin, "(%d) %s:", (uint32_t) path_len, from_name);
//     }
//     StrHelper::strncpy(p->msg, msg, sizeof(p->msg));
//   }

//   int render(DisplayDriver& display) override {
//     char tmp[16];
//     display.setCursor(0, 0);
//     display.setTextSize(1);
//     display.setColor(DisplayDriver::GREEN);
//     sprintf(tmp, "Unread: %d", num_unread);
//     display.print(tmp);

//     auto p = &unread[0];

//     int secs = _rtc->getCurrentTime() - p->timestamp;
//     if (secs < 60) {
//       sprintf(tmp, "%ds", secs);
//     } else if (secs < 60*60) {
//       sprintf(tmp, "%dm", secs / 60);
//     } else {
//       sprintf(tmp, "%dh", secs / (60*60));
//     }
//     display.setCursor(display.width() - display.getTextWidth(tmp) - 2, 0);
//     display.print(tmp);

//     display.drawRect(0, 11, display.width(), 1);  // horiz line

//     display.setCursor(0, 14);
//     display.setColor(DisplayDriver::YELLOW);
//     char filtered_origin[sizeof(p->origin)];
//     display.translateUTF8ToBlocks(filtered_origin, p->origin, sizeof(filtered_origin));
//     display.print(filtered_origin);

//     display.setCursor(0, 25);
//     display.setColor(DisplayDriver::LIGHT);
//     char filtered_msg[sizeof(p->msg)];
//     display.translateUTF8ToBlocks(filtered_msg, p->msg, sizeof(filtered_msg));
//     display.printWordWrap(filtered_msg, display.width());

// #if AUTO_OFF_MILLIS==0 // probably e-ink
//     return 10000; // 10 s
// #else
//     return 1000;  // next render after 1000 ms
// #endif
//   }

//   bool handleInput(char c) override {
//     if (c == KEY_NEXT || c == KEY_RIGHT) {
//       num_unread--;
//       if (num_unread == 0) {
//         _task->gotoHomeScreen();
//       } else {
//         // delete first/curr item from unread queue
//         for (int i = 0; i < num_unread; i++) {
//           unread[i] = unread[i + 1];
//         }
//       }
//       return true;
//     }
//     if (c == KEY_ENTER) {
//       num_unread = 0;  // clear unread queue
//       _task->gotoHomeScreen();
//       return true;
//     }
//     return false;
//   }
// };