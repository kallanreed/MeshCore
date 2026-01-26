#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <helpers/RefCountedDigitalPin.h>

#define CARDKB_ADDR 0x5F

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
    if (!_begun)
      return 0;

    Wire.requestFrom(CARDKB_ADDR, 1);

    // TODO: handle multiple bytes with a buffer.
    if (Wire.available())
      return Wire.read();

    return 0;
  }
};
