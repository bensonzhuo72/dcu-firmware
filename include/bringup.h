#pragma once

#include <Arduino.h>
#include <IPAddress.h>

#include "can_bus.h"

// Bench bring-up test. Exercises everything except the control outputs
// (precharge, DSR, remote, triggers), which io_init() leaves held low.
//
// Every BRINGUP_PERIOD_MS:
//  - blue and yellow status LEDs toggle (on 500 ms / off 500 ms)
//  - CAN0 sends ID BRINGUP_CAN0_TX_ID, CAN1 sends ID BRINGUP_CAN1_TX_ID
//  - a UDP heartbeat is broadcast to BRINGUP_UDP_HEARTBEAT_PORT
//  - mission, inputs and counters are printed to USB and to the UART (Serial5)
// Received UDP packets are echoed back to the sender.

constexpr uint32_t BRINGUP_PERIOD_MS = 1000;
constexpr uint32_t BRINGUP_CAN0_TX_ID = 1;
constexpr uint32_t BRINGUP_CAN1_TX_ID = 2;
constexpr uint16_t BRINGUP_UDP_HEARTBEAT_PORT = 5001;

void bringup_init();
void bringup_poll();  // call from loop(); never blocks

// Called from the application's on_*() handlers.
void bringup_on_can(Bus bus, uint32_t id);
void bringup_on_udp(const uint8_t *data, size_t len,
                    const IPAddress &from_ip, uint16_t from_port);
