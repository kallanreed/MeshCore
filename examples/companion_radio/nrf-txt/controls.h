#pragma once

#include <stdio.h>
#include <helpers/ui/UIScreen.h>
#include "icons.h"
#include "ui_view_model.h"

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
    display.drawTextLeftAlign(3, 5, "Q to shutdown");
    display.drawTextLeftAlign(3, 25, "G to toggle GPS");
    display.drawTextLeftAlign(3, 45, "B to toggle Buzzer");

    sprintf(_pin_code, "Pin: %d", _model->getBlePin());
    display.drawTextLeftAlign(3, 65, _pin_code);
  }
};

class PowerPage : public UIPage {
public:
  PowerPage(UIViewModel* model) : UIPage(model) {}

  const uint8_t* getIcon() override {
    return icon_power;
  }

  void renderPreview(DisplayDriver& display) override {
    display.drawTextCentered(display.width() / 2, 45, "Enter to Shutdown");

    char tmp[30];
    auto total_min = _model->getUptimeMin();
    auto hours = total_min / 60;
    auto minutes = total_min % 60;
    sprintf(tmp, "Uptime %dH:%dM", hours, minutes);
    display.drawTextCentered(display.width() / 2, 65, tmp);
  }

  void activate() override {
    _model->shutdown(false);
  }
};