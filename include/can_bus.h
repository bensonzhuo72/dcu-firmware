#pragma once

#include <Arduino.h>

// FlexCAN_T4 is header-only and is included only in can_bus.cpp, so it is
// never pulled into more than one translation unit.
//
// The CAN RX interrupt copies frames into a 256-frame queue per bus;
// can_poll() drains the queues in the main loop and calls on_can_frame().
// can_send() never blocks: it uses a free mailbox or a 16-frame TX queue.

constexpr uint32_t CAN0_BAUD = 500000;
constexpr uint32_t CAN1_BAUD = 500000;

// Bus names follow the schematic:
//   Bus::CAN0 -> Teensy FlexCAN CAN1 (pins 22 TX / 23 RX)
//   Bus::CAN1 -> Teensy FlexCAN CAN2 (pins 1 TX / 0 RX)
enum class Bus : uint8_t { CAN0, CAN1 };

void can_init();
void can_poll();

// Queue a frame (len <= 8, standard ID unless extended).
// Returns false if the TX queue is full.
bool can_send(Bus bus, uint32_t id, const uint8_t *data, uint8_t len,
              bool extended = false);

// Implemented by the application. Called from the main loop.
void on_can_frame(Bus bus, uint32_t id, bool extended,
                  const uint8_t *data, uint8_t len);
