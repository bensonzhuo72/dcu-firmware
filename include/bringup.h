#pragma once

#include <Arduino.h>
#include <IPAddress.h>

#include "can_bus.h"

// Bench bring-up test. Drives TRIG_1_CTRL, TRIG_2_CTRL and DSR_CTRL (relay)
// high at start-up. Precharge and remote stay held low by io_init().
//
// Blue and yellow status LEDs flash together (on 500 ms / off 500 ms).
//
// Every BRINGUP_PERIOD_MS:
//  - a UDP heartbeat is broadcast to BRINGUP_UDP_HEARTBEAT_PORT
//  - mission, inputs and counters are printed to USB and to the UART (Serial5)
// Received UDP packets are echoed back to the sender.
//
// The CAN link test lives in can_test.h; bringup runs it.

constexpr uint32_t BRINGUP_PERIOD_MS = 1000;
constexpr uint16_t BRINGUP_UDP_HEARTBEAT_PORT = 5001;

// Prints one line to USB and the UART link (subject to LOG_CAN_ONLY).
// Never blocks.
void bringup_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void bringup_init();
void bringup_poll();  // call from loop(); never blocks

// Called from the application's on_*() handlers.
void bringup_on_can(Bus bus, uint32_t id, bool extended,
                    const uint8_t *data, uint8_t len);
void bringup_on_udp(const uint8_t *data, size_t len,
                    const IPAddress &from_ip, uint16_t from_port);
