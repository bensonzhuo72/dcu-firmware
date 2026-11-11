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

// Hardware RX filter: both buses accept only these standard IDs (at most 8).
// Currently the partner Teensy's echoes and heartbeats from the CAN test
// (see can_test.h). Both buses accept all four so crossed wiring shows up.
constexpr uint32_t CAN_RX_IDS[] = {0x101, 0x102, 0x201, 0x202};

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

// Controller error state. Counters and state are current; the flags are every
// error type seen since the previous call for that bus.
enum class CanState : uint8_t { Active, Passive, BusOff };

struct CanErrors {
  uint8_t tec = 0;  // transmit error counter
  uint8_t rec = 0;  // receive error counter
  CanState state = CanState::Active;
  bool ack = false;    // sent a frame, nobody acknowledged it
  bool bit0 = false;   // sent dominant, read back recessive
  bool bit1 = false;   // sent recessive, read back dominant
  bool crc = false;
  bool form = false;
  bool stuff = false;
};

CanErrors can_errors(Bus bus);

// Implemented by the application. Called from the main loop.
void on_can_frame(Bus bus, uint32_t id, bool extended,
                  const uint8_t *data, uint8_t len);
