#pragma once

#include <Arduino.h>
#include <Wire.h>

#define NRF_TXT_KEYBOARD_ADDR 0x5F

#ifndef NRF_TXT_KEYBOARD_POLL_MS
  #define NRF_TXT_KEYBOARD_POLL_MS 20
#endif

static inline char nrfTxtKeyboardPoll() {
  static uint32_t next_poll = 0;
  if ((int32_t)(millis() - next_poll) < 0) {
    return 0;
  }
  next_poll = millis() + NRF_TXT_KEYBOARD_POLL_MS;

  Wire.requestFrom((uint8_t)NRF_TXT_KEYBOARD_ADDR, (uint8_t)1);
  if (Wire.available()) {
    return (char)Wire.read();
  }
  return 0;
}
