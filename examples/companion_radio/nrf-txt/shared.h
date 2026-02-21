#pragma once

#include <Arduino.h>
#include <stdio.h>

static inline void formatAgeSeconds(char* out, size_t size, uint32_t age_sec) {
  if (age_sec < 60) {
    snprintf(out, size, "%lus", static_cast<unsigned long>(age_sec));
  } else if (age_sec < 60 * 60) {
    snprintf(out, size, "%lum", static_cast<unsigned long>(age_sec / 60));
  } else if (age_sec < 60 * 60 * 24) {
    snprintf(out, size, "%luh", static_cast<unsigned long>(age_sec / (60 * 60)));
  } else {
    snprintf(out, size, "%lud", static_cast<unsigned long>(age_sec / (60 * 60 * 24)));
  }
}

static inline void formatAgeMillis(char* out, size_t size, uint32_t timestamp_ms) {
  uint32_t age_sec = (millis() - timestamp_ms) / 1000;
  formatAgeSeconds(out, size, age_sec);
}
