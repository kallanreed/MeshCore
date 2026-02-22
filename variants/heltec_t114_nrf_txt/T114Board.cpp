#include "T114Board.h"

#include <Arduino.h>
#include <Wire.h>
#include <helpers/RefCountedDigitalPin.h>

extern RefCountedDigitalPin vext_power;

// Static configuration for power management
// Values come from variant.h defines
const PowerMgtConfig power_config = {
  .lpcomp_ain_channel = PWRMGT_LPCOMP_AIN,
  .lpcomp_refsel = PWRMGT_LPCOMP_REFSEL,
  .voltage_bootlock = PWRMGT_VOLTAGE_BOOTLOCK
};

void T114Board::initiateShutdown(uint8_t reason) {
  disablePeripheralPower();
  digitalWrite(SX126X_POWER_EN, LOW);

  bool enable_lpcomp = (reason == SHUTDOWN_REASON_LOW_VOLTAGE ||
                        reason == SHUTDOWN_REASON_BOOT_PROTECT);
  pinMode(PIN_BAT_CTL, OUTPUT);
  digitalWrite(PIN_BAT_CTL, enable_lpcomp ? HIGH : LOW);

  if (enable_lpcomp) {
    configureVoltageWake(power_config.lpcomp_ain_channel, power_config.lpcomp_refsel);
  }

  enterSystemOff(reason);
}

void T114Board::disablePeripheralPower() {
  pinMode(PIN_3V3_EN, OUTPUT);
  digitalWrite(PIN_3V3_EN, LOW);
}

void T114Board::enablePeripheralPower() {
  pinMode(PIN_3V3_EN, OUTPUT);
  digitalWrite(PIN_3V3_EN, HIGH);
}

void T114Board::powerOff() {
  digitalWrite(LED_PIN, HIGH);
  disablePeripheralPower();
  sd_power_system_off();
}

void T114Board::onBeforeTransmit() {
  digitalWrite(P_LORA_TX_LED, LOW);   // turn TX LED on
}

void T114Board::onAfterTransmit() {
  digitalWrite(P_LORA_TX_LED, HIGH);   // turn TX LED off
}

uint16_t T114Board::getBattMilliVolts() {
  int adc_value = 0;
  analogReadResolution(ADC_RESOLUTION);
  analogReference(AR_INTERNAL_3_0);
  pinMode(PIN_BAT_CTL, OUTPUT); // battery adc can be read only ctrl pin 6 set to high
  digitalWrite(PIN_BAT_CTL, 1);

  delay(10);
  adc_value = analogRead(PIN_VBAT_READ);
  digitalWrite(PIN_BAT_CTL, 0);

  // 3000mV / 4096 (12 bits) = .732mV/bit
  constexpr auto adc_scale = ADC_MULTIPLIER * AREF_VOLTAGE * 1000.0f / (1 << ADC_RESOLUTION);
  return (uint16_t)(adc_value * adc_scale);
}

void T114Board::begin() {
  NRF52Board::begin();

  pinMode(PIN_VBAT_READ, INPUT);

  Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
  Wire.begin();

  pinMode(P_LORA_TX_LED, OUTPUT);
  digitalWrite(P_LORA_TX_LED, HIGH);

  pinMode(SX126X_POWER_EN, OUTPUT);
  // Boot voltage protection check (may not return if voltage too low)
  // We need to call this after we configure SX126X_POWER_EN as output but before we pull high
  checkBootVoltage(&power_config);
  digitalWrite(SX126X_POWER_EN, HIGH);

  // Set up the shared Vext control pin.
  vext_power.begin();
  enablePeripheralPower();

  // give peripherals some time to power up.
  delay(10); 
}
