#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <helpers/RefCountedDigitalPin.h>
#include <helpers/ui/DisplayDriver.h>
#include <helpers/ui/ST7789Spi.h>

#define CARDKB_ADDR 0x5F

enum class KeyCode {
  FN_B = 0xaa,
  FN_G = 0x9e,
  FN_Q = 0x8d
};

class CardKB {
  RefCountedDigitalPin* _vext_power;
  bool _begun = false;

public:
  explicit CardKB(RefCountedDigitalPin* vext_power) : _vext_power(vext_power) { }

  ~CardKB() {
    end();
  }

  void begin() {
    if (_begun || !_vext_power)
      return;
    _vext_power->claim();
    _begun = true;
  }

  void end() {
    if (!_begun || !_vext_power)
      return;
    _vext_power->release();
    _begun = false;
  }

  uint8_t readKeyboard() {
    uint8_t kb = 0;

    if (!_begun)
      return kb;

    if(Wire.requestFrom(CARDKB_ADDR, 1)) {
      if (Wire.available())
        kb = Wire.read();
    }

    return kb;
  }
};

class ST7789DisplayNrfTxt : public DisplayDriver {
  ST7789Spi display;
  bool _isOn = false;
  uint16_t _color = ST77XX_WHITE;
  int _x = 0;
  int _y = 0;

public:
  ST7789DisplayNrfTxt()
    : DisplayDriver(240, 135),
      display(&SPI1, PIN_TFT_RST, PIN_TFT_DC, PIN_TFT_CS, GEOMETRY_RAWMODE, 240, 135) {}

  bool begin();
  bool isOn() override { return _isOn; }
  void turnOn() override;
  void turnOff() override;
  void clear() override;
  void startFrame(Color bkg = DARK) override;
  void setTextSize(int sz) override;
  void setColor(Color c) override;
  void setCursor(int x, int y) override;
  void print(const char* str) override;
  void printWordWrap(const char* str, int max_width) override;
  void fillRect(int x, int y, int w, int h) override;
  void drawRect(int x, int y, int w, int h) override;
  void drawXbm(int x, int y, const uint8_t* bits, int w, int h) override;
  void drawXbm(int x, int y, const uint8_t* bits, int w, int h, int scale) override;
  uint16_t getTextWidth(const char* str) override;
  void endFrame() override;
};
