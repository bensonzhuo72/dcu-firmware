#pragma once

#include <Arduino.h>

// Teensy 4.1 pin assignments, taken from docs/dcu-schematic.png.

// Mission inputs: active low, internal pull-up, edge interrupt
constexpr uint8_t PIN_MISSION_1_EV = 7;
constexpr uint8_t PIN_MISSION_2_D  = 6;
constexpr uint8_t PIN_MISSION_3_D  = 5;
constexpr uint8_t PIN_MISSION_4_D  = 4;
constexpr uint8_t PIN_MISSION_5_D  = 3;
constexpr uint8_t PIN_MISSION_6_D  = 2;

// Status inputs
constexpr uint8_t PIN_DSMS = 8;
constexpr uint8_t PIN_TSAB = 33;

// Analog input (A13)
constexpr uint8_t PIN_BP_3V3 = 27;

// LED PWM outputs. Pins 36 and 37 share FlexPWM2 submodule 3, so they
// always run at the same PWM frequency.
constexpr uint8_t PIN_LED_BLUE_CTRL   = 37;
constexpr uint8_t PIN_LED_YELLOW_CTRL = 36;

// Trigger outputs (fast slew)
constexpr uint8_t PIN_TRIG_1_CTRL = 35;
constexpr uint8_t PIN_TRIG_2_CTRL = 34;

// Digital outputs
constexpr uint8_t PIN_PRECHARGE_EN = 26;
constexpr uint8_t PIN_DSR_CTRL     = 9;
constexpr uint8_t PIN_REMOTE_CTRL  = 10;

// CAN. The schematic's "CAN0" maps to Teensy FlexCAN CAN1 (pins 22/23),
// and "CAN1" maps to Teensy FlexCAN CAN2 (pins 0/1). The FlexCAN_T4 library
// configures these pins itself.
constexpr uint8_t PIN_CAN0_TX = 22;
constexpr uint8_t PIN_CAN0_RX = 23;
constexpr uint8_t PIN_CAN1_TX = 1;
constexpr uint8_t PIN_CAN1_RX = 0;

// TEENSY_TX / TEENSY_RX are Serial5 (pin 20 TX5, pin 21 RX5).
constexpr uint8_t PIN_TEENSY_TX = 20;
constexpr uint8_t PIN_TEENSY_RX = 21;

// Ethernet uses the Teensy 4.1's onboard PHY (T+/T-/R+/R- header pads),
// so there are no GPIO pins to configure. LED_GREEN is driven by the PHY.
