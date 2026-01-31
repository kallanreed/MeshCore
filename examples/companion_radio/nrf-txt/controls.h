#pragma once

#include <stdio.h>
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

  // Called when "Enter" is pressed while selected.
  virtual void activate() {};
};

class HomePage : public UIPage {
  char _pin_code[12];

public:
  HomePage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_home;
  }

  void renderPreview(DisplayDriver& display) override {
    display.setTextSize(1);
    display.drawTextLeftAlign(3, 5, "Q to shutdown");
    display.setTextSize(2);
    display.drawTextLeftAlign(3, 25, "G to toggle GPS");
    display.setTextSize(3);
    display.drawTextLeftAlign(3, 45, "B to toggle Buzzer");

    sprintf(_pin_code, "Pin: %d", _model->getBlePin());
    display.drawTextLeftAlign(3, 65, _pin_code);
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
    display.drawTextCentered(display.width() / 2, 55, tmp);

    display.setTextSize(1);
    display.drawTextCentered(display.width() / 2, 100, "Enter to Toggle");
  }

  void activate() override {
    _model->toggleGPS();
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

    if (dt.second % 2) {
      display.fillRect(79, 50, 3, 3);
      display.fillRect(79, 75, 3, 3);
      display.fillRect(159, 50, 3, 3);
      display.fillRect(159, 75, 3, 3);
    }

    sprintf(tmp, "%d/%d/%d", dt.month, dt.day, dt.year);
    display.drawTextCentered(center_x, 1, tmp);
    // display.setColor(DisplayDriver::INVERSE);
    // display.fillRect(0, 0, 240, 18);
    // display.setColor(DisplayDriver::LIGHT);
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

    if (hours > 0)
      sprintf(tmp, "Uptime: %dH %dM", hours, minutes);
    else
      sprintf(tmp, "Uptime: %dM", minutes);

    display.setTextSize(3);
    display.drawTextCentered(display.width() / 2, 40, tmp);

    display.setTextSize(1);
    display.drawTextCentered(display.width() / 2, 70, "Enter to Shutdown");
  }

  void activate() override {
    _model->shutdown(false);
  }
};