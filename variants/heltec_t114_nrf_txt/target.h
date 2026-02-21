#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <T114Board.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/RefCountedDigitalPin.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/sensors/EnvironmentSensorManager.h>
#include <helpers/sensors/LocationProvider.h>

#include <helpers/ui/MomentaryButton.h>
#include <nrf_hardware.h>

extern T114Board board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern EnvironmentSensorManager sensors;
extern RefCountedDigitalPin vext_power;

extern DISPLAY_CLASS display;
extern MomentaryButton user_btn;

bool radio_init();
uint32_t radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(int8_t dbm);
mesh::LocalIdentity radio_new_identity();
