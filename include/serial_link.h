#pragma once

#include <Arduino.h>

// Serial5 link on TEENSY_TX (pin 20) / TEENSY_RX (pin 21).
//
// The UART interrupt fills Serial5's RX buffer; serial_link_poll() drains it
// from the main loop and calls on_link_line() for each complete line.
// TX is buffered and serial_link_send() never waits for space.

constexpr uint32_t TEENSY_SERIAL_BAUD = 115200;
constexpr size_t   LINK_LINE_MAX = 128;

void serial_link_init();
void serial_link_poll();

// Queues data for transmit. Returns false (and sends nothing) if the TX
// buffer does not have room for all of it.
bool serial_link_send(const uint8_t *data, size_t len);

// Implemented by the application. Called from the main loop with a
// NUL-terminated line (without the line ending).
void on_link_line(const char *line, size_t len);
