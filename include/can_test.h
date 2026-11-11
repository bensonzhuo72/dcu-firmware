#pragma once

#include <Arduino.h>

#include "can_bus.h"

// CAN link test against the partner Teensy (tools/can_partner/can_partner.ino).
//
// Every CAN_TEST_PERIOD_MS, each enabled bus sends a ping. The partner echoes
// it back with ID + 0x100 and also sends its own heartbeat. At the end of each
// period the DCU prints ONE line per bus:
//
//   [CAN1] #12 PASS    echo 412us  hb 1 | OK          TEC=0   REC=0
//   [CAN1] #13 NO-ECHO             hb 0 | BUS-OFF     TEC=248 REC=0  BIT0
//   [CAN1]   -> DCU can't see its own frames: check transceiver power/standby, ...
//
// The "->" diagnosis line only prints when the diagnosis changes. Totals print
// every CAN_TEST_TOTALS_EVERY periods. Unexpected frames print as they arrive.
//
// IDs (standard 11-bit, 8 bytes, 500 kbit/s):
//              DCU ping   partner echo   partner heartbeat
//   CAN0       0x001      0x101          0x201 or 0x202
//   CAN1       0x002      0x102          0x201 or 0x202
// Ping/echo data: [0..3] sequence, [4..7] DCU uptime ms (echo copies the ping).
// Heartbeat data: [0..3] sequence, [4..7] partner uptime ms.

// Which buses to test: [0] = CAN0 (pins 22/23), [1] = CAN1 (pins 0/1).
constexpr bool CAN_TEST_BUS_ENABLED[2] = {false, true};

constexpr uint32_t CAN_TEST_PERIOD_MS = 2000;
constexpr uint32_t CAN_TEST_TOTALS_EVERY = 10;  // periods (20 s)

constexpr uint32_t CAN_TEST_PING_ID[2] = {0x001, 0x002};
constexpr uint32_t CAN_TEST_ECHO_ID[2] = {0x101, 0x102};
constexpr uint32_t CAN_TEST_HB_ID[2]   = {0x201, 0x202};

void can_test_init();
void can_test_poll();  // call from loop(); never blocks
void can_test_on_frame(Bus bus, uint32_t id, bool extended,
                       const uint8_t *data, uint8_t len);
