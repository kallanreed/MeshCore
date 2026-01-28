#include "nrf_hardware.h"

static constexpr int kXOffset = 0;
static constexpr int kYOffset = 1;

bool ST7789DisplayNrfTxt::begin() {
  if(!_isOn) {
    pinMode(PIN_TFT_VDD_CTL, OUTPUT);
    pinMode(PIN_TFT_LEDA_CTL, OUTPUT);
    digitalWrite(PIN_TFT_VDD_CTL, LOW);
  #ifdef PIN_TFT_LEDA_CTL_ACTIVE
    digitalWrite(PIN_TFT_LEDA_CTL, PIN_TFT_LEDA_CTL_ACTIVE);
  #else
    digitalWrite(PIN_TFT_LEDA_CTL, LOW);
  #endif
    digitalWrite(PIN_TFT_RST, HIGH);

    display.init();
    display.landscapeScreen();
    display.displayOn();
    setCursor(0,0);

    _isOn = true;
  }
  return true;
}

void ST7789DisplayNrfTxt::turnOn() {
  if (!_isOn) {
    digitalWrite(PIN_TFT_VDD_CTL, LOW);
    digitalWrite(PIN_TFT_RST, HIGH);

    display.init();
    display.displayOn();
    delay(20);

  #ifdef PIN_TFT_LEDA_CTL_ACTIVE
    digitalWrite(PIN_TFT_LEDA_CTL, PIN_TFT_LEDA_CTL_ACTIVE);
  #else
    digitalWrite(PIN_TFT_LEDA_CTL, LOW);
  #endif
    _isOn = true;
  }
}

void ST7789DisplayNrfTxt::turnOff() {
  digitalWrite(PIN_TFT_VDD_CTL, HIGH);
#ifdef PIN_TFT_LEDA_CTL_ACTIVE
  digitalWrite(PIN_TFT_LEDA_CTL, !PIN_TFT_LEDA_CTL_ACTIVE);
#else
  digitalWrite(PIN_TFT_LEDA_CTL, HIGH);
#endif
  digitalWrite(PIN_TFT_RST, LOW);
  _isOn = false;
}

void ST7789DisplayNrfTxt::clear() {
  display.clear();
}

void ST7789DisplayNrfTxt::startFrame(Color bkg) {
  display.clear();
  _color = ST77XX_WHITE;
  display.setRGB(_color);
  display.setFont(ArialMT_Plain_16);
}

void ST7789DisplayNrfTxt::setTextSize(int sz) {
  switch(sz) {
    case 1 :
      display.setFont(ArialMT_Plain_16);
      break;
    case 2 :
      display.setFont(ArialMT_Plain_24);
      break;
    default:
      display.setFont(ArialMT_Plain_16);
  }
}

void ST7789DisplayNrfTxt::setColor(Color c) {
  switch (c) {
    case DisplayDriver::DARK :
      _color = ST77XX_BLACK;
      display.setColor(OLEDDISPLAY_COLOR::BLACK);
      break;
    default:
      _color = ST77XX_WHITE;
      display.setColor(OLEDDISPLAY_COLOR::WHITE);
      break;
  }
  display.setRGB(_color);
}

void ST7789DisplayNrfTxt::setCursor(int x, int y) {
  _x = x + kXOffset;
  _y = y + kYOffset;
}

void ST7789DisplayNrfTxt::print(const char* str) {
  display.drawString(_x, _y, str);
}

void ST7789DisplayNrfTxt::printWordWrap(const char* str, int max_width) {
  display.drawStringMaxWidth(_x, _y, max_width, str);
}

void ST7789DisplayNrfTxt::fillRect(int x, int y, int w, int h) {
  display.fillRect(x + kXOffset, y + kYOffset, w, h);
}

void ST7789DisplayNrfTxt::drawRect(int x, int y, int w, int h) {
  display.drawRect(x + kXOffset, y + kYOffset, w, h);
}

void ST7789DisplayNrfTxt::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  drawXbm(x, y, bits, w, h, 1);
}

void ST7789DisplayNrfTxt::drawXbm(int x, int y, const uint8_t* bits, int w, int h, int scale) {
  if (scale < 1) {
    return;
  }

  uint16_t startX = x + kXOffset;
  uint16_t startY = y + kYOffset;
  uint16_t widthInBytes = (w + 7) / 8;

  for (uint16_t by = 0; by < h; by++) {
    int y1 = startY + (int)(by * scale);
    for (uint16_t bx = 0; bx < w; bx++) {
      int x1 = startX + (int)(bx * scale);
      uint16_t byteOffset = (by * widthInBytes) + (bx / 8);
      uint8_t bitMask = 0x80 >> (bx & 7);
      bool bitSet = pgm_read_byte(bits + byteOffset) & bitMask;

      if (bitSet) {
        display.fillRect(x1, y1, scale, scale);
      }
    }
  }
}

uint16_t ST7789DisplayNrfTxt::getTextWidth(const char* str) {
  return display.getStringWidth(str);
}

void ST7789DisplayNrfTxt::endFrame() {
  display.display();
}
