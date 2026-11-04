#pragma once

#include <Arduino.h>

// ---- Mission selection ------------------------------------------------------
// Mission pins idle high. The pin pulled low is the selected mission; if no
// pin is low, no mission is selected. More than one low pin is reported as
// Invalid.
enum class Mission : uint8_t {
  None = 0,
  M1_EV,
  M2,
  M3,
  M4,
  M5,
  M6,
  Invalid,
};

constexpr uint8_t MISSION_PIN_COUNT = 6;

// Pins must be quiet (no edges) for this long before the state is accepted.
constexpr uint32_t MISSION_DEBOUNCE_MS = 20;

// The pin interrupt only records that an edge happened and when; this
// function does the debounce and decoding in the main loop.
// Call from loop(). Returns true when the debounced selection changes.
bool mission_update();

Mission mission_selected();
const char *mission_name(Mission m);

// ---- LED PWM ----------------------------------------------------------------
constexpr float    LED_PWM_FREQ_HZ = 1000.0f;
constexpr uint8_t  LED_PWM_BITS    = 12;
constexpr uint16_t LED_PWM_MAX     = (1u << LED_PWM_BITS) - 1;

// Duty cycle is 0..LED_PWM_MAX.
void led_set_blue(uint16_t duty);
void led_set_yellow(uint16_t duty);

// ---- BP_3.3V analog input ---------------------------------------------------
constexpr uint8_t ADC_BITS = 12;

uint16_t bp3v3_read_raw();
// Voltage at the Teensy pin (0..3.3 V). Scale for any divider on the board.
float bp3v3_read_volts();

// ---- Digital I/O ------------------------------------------------------------
bool dsms_read();
bool tsab_read();

void trig1_write(bool level);
void trig2_write(bool level);
void precharge_en_write(bool level);
void dsr_ctrl_write(bool level);
void remote_ctrl_write(bool level);

// ---- Init -------------------------------------------------------------------
void io_init();
