#pragma once

#include "WVariant.h"

////////////////////////////////////////////////////////////////////////////////
// Low frequency clock source

#define USE_LFXO    // 32.768 kHz crystal oscillator
#define VARIANT_MCK (64000000ul)

////////////////////////////////////////////////////////////////////////////////
// Power

#define NRF_APM
#define PIN_3V3_EN              (21)

#define PIN_VBAT_READ           (4)
#define PIN_BAT_CTL             (6)
#define BATTERY_PIN             PIN_VBAT_READ
#define ADC_MULTIPLIER          (4.90F)

#define ADC_RESOLUTION          (12)
#define BATTERY_SENSE_RES       (12)

#define AREF_VOLTAGE            (3.0)

// Power management boot protection threshold (millivolts)
// Set to 0 to disable boot protection
#define PWRMGT_VOLTAGE_BOOTLOCK 3300   // Won't boot below this voltage (mV)
// LPCOMP wake configuration (voltage recovery from SYSTEMOFF)
// AIN2 = P0.04 = BATTERY_PIN / PIN_VBAT_READ
#define PWRMGT_LPCOMP_AIN 2
#define PWRMGT_LPCOMP_REFSEL 1  // 2/8 VDD (~3.68-4.04V)

////////////////////////////////////////////////////////////////////////////////
// Number of pins

#define PINS_COUNT              (48)
#define NUM_DIGITAL_PINS        (48)
#define NUM_ANALOG_INPUTS       (1)
#define NUM_ANALOG_OUTPUTS      (0)

////////////////////////////////////////////////////////////////////////////////
// UART pin definition

#define PIN_SERIAL1_RX          (37)
#define PIN_SERIAL1_TX          (39)

#define PIN_SERIAL2_RX          (9)
#define PIN_SERIAL2_TX          (10)

////////////////////////////////////////////////////////////////////////////////
// I2C pin definition

#define WIRE_INTERFACES_COUNT 	(1)

#define PIN_WIRE_SDA            (16) // P0.16
#define PIN_WIRE_SCL            (13) // P0.13

#define PIN_BOARD_SDA           PIN_WIRE_SDA
#define PIN_BOARD_SCL           PIN_WIRE_SCL

////////////////////////////////////////////////////////////////////////////////
// SPI pin definition

#define SPI_INTERFACES_COUNT    (2)
#define PIN_SPI_NSS             (24)

#define PIN_SPI_MISO            (23)
#define PIN_SPI_MOSI            (22)
#define PIN_SPI_SCK             (19)

#define PIN_SPI1_MISO           (43)
#define PIN_SPI1_MOSI           (41)
#define PIN_SPI1_SCK            (40)

////////////////////////////////////////////////////////////////////////////////
// Builtin LEDs

#define LED_BUILTIN             (-1)           // (35) flash_cache.c blinks this on write. Very annoying.
#define PIN_LED                 (35)
#define LED_PIN                 PIN_LED
#define LED_RED                 PIN_LED
#define LED_BLUE                (-1)           // No blue led, prevents Bluefruit flashing the green LED during advertising.

#define LED_STATE_ON            LOW

#define PIN_NEOPIXEL            (14)
#define NEOPIXEL_NUM            (2)

////////////////////////////////////////////////////////////////////////////////
// Builtin buttons

#define PIN_BUTTON1             (42)
#define BUTTON_PIN              PIN_BUTTON1

// #define PIN_BUTTON2             (11)
// #define BUTTON_PIN2             PIN_BUTTON2

#define PIN_USER_BTN            BUTTON_PIN

#define EXTERNAL_FLASH_DEVICES  MX25R1635F
#define EXTERNAL_FLASH_USE_QSPI

////////////////////////////////////////////////////////////////////////////////
// Lora

#define USE_SX1262
#define LORA_CS                 PIN_SPI_NSS
#define SX126X_DIO1             (20)
#define SX126X_BUSY             (17)
#define SX126X_RESET            (25)
#define SX126X_DIO2_AS_RF_SWITCH
#define SX126X_DIO3_TCXO_VOLTAGE 1.8
#define SX126X_POWER_EN         (7)
#define SX126X_CURRENT_LIMIT    (140)
#define SX126X_RX_BOOSTED_GAIN  (1)

#define P_LORA_NSS              PIN_SPI_NSS
#define P_LORA_DIO_1            SX126X_DIO1
#define P_LORA_RESET            SX126X_RESET
#define P_LORA_BUSY             SX126X_BUSY
#define P_LORA_SCLK             (19)
#define P_LORA_MOSI             (22)
#define P_LORA_MISO             (23)
#define P_LORA_TX_LED           (35)

////////////////////////////////////////////////////////////////////////////////
// Buzzer

// NB: Needs to be defined on the command line.
//#define PIN_BUZZER              (33)

////////////////////////////////////////////////////////////////////////////////
// GPS

#define GPS_EN                  (34)
#define GPS_RESET               (38)
#define PIN_GPS_RX              (39)  // This is for bits going TOWARDS the GPS
#define PIN_GPS_TX              (37)  // This is for bits going TOWARDS the CPU
#define PIN_GPS_EN              GPS_EN
#define PIN_GPS_RESET           GPS_RESET
#define PIN_GPS_RESET_ACTIVE    LOW

////////////////////////////////////////////////////////////////////////////////
// TFT

#define PIN_TFT_SCL             (40)
#define PIN_TFT_SDA             (41)
#define PIN_TFT_RST             (2)
#define PIN_TFT_VDD_CTL         (3)
#define PIN_TFT_LEDA_CTL        (15)
#define PIN_TFT_CS              (11)
#define PIN_TFT_DC              (12)
