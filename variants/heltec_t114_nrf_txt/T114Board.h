#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include <helpers/NRF52Board.h>

class T114Board : public NRF52BoardDCDC {
protected:
  void enablePeripheralPower();
  void disablePeripheralPower();
  void initiateShutdown(uint8_t reason) override;

public:
  T114Board() : NRF52Board("T114_OTA") {}
  void begin();

  void onBeforeTransmit() override;
  void onAfterTransmit() override;

  uint16_t getBattMilliVolts() override;

  const char* getManufacturerName() const override {
    return "Heltec T114";
  }

  void powerOff() override;
};
