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

static inline bool nrfIsHexDigit(char c) {
  return (c >= '0' && c <= '9')
      || (c >= 'a' && c <= 'f')
      || (c >= 'A' && c <= 'F');
}

static inline uint8_t nrfHexDigitValue(char c) {
  if (c >= '0' && c <= '9')
    return static_cast<uint8_t>(c - '0');
  if (c >= 'a' && c <= 'f')
    return static_cast<uint8_t>(10 + (c - 'a'));
  return static_cast<uint8_t>(10 + (c - 'A'));
}

static inline bool formatPathText(
  const uint8_t* path,
  int path_len,
  char* out,
  uint8_t out_size) {
  if (!out || out_size == 0)
    return false;

  out[0] = 0;
  if (!path || path_len <= 0)
    return true;

  size_t pos = 0;
  for (int i = 0; i < path_len; i++) {
    int written = snprintf(
      out + pos,
      out_size - pos,
      i == 0 ? "%02x" : ",%02x",
      path[i]);
    if (written < 0 || static_cast<size_t>(written) >= (out_size - pos))
      return false;
    pos += static_cast<size_t>(written);
  }

  return true;
}

static inline bool parsePathText(
  const char* text,
  uint8_t* out_path,
  uint8_t* out_len,
  uint8_t max_path_len) {
  if (!text || !out_path || !out_len || max_path_len == 0)
    return false;

  uint8_t count = 0;
  const char* p = text;

  while (*p) {
    while (*p == ' ' || *p == '\t')
      p++;

    if (*p == 0)
      break;
    if (!nrfIsHexDigit(p[0]) || !nrfIsHexDigit(p[1]))
      return false;
    if (count >= max_path_len)
      return false;

    out_path[count++] = static_cast<uint8_t>(
      (nrfHexDigitValue(p[0]) << 4) | nrfHexDigitValue(p[1]));
    p += 2;

    while (*p == ' ' || *p == '\t')
      p++;
    if (*p == 0)
      break;
    if (*p != ',')
      return false;
    p++;
  }

  if (count == 0)
    return false;

  *out_len = count;
  return true;
}

static inline bool normalizeHashtagName(
  const char* input,
  char* out,
  uint8_t out_size) {
  if (!input || !out || out_size < 2)
    return false;

  while (*input == ' ' || *input == '\t')
    input++;

  size_t len = strlen(input);
  while (len > 0 && (input[len - 1] == ' ' || input[len - 1] == '\t'))
    len--;
  if (len == 0)
    return false;

  size_t start = (input[0] == '#') ? 1 : 0;
  if (len == start)
    return false;
  if ((len - start) + 2 > out_size)
    return false;

  out[0] = '#';
  memcpy(out + 1, input + start, len - start);
  out[1 + (len - start)] = 0;
  return true;
}
