#pragma once

#include <Arduino.h>

// Non-blocking debug logging over USB Serial.
//
// Serial.print() on the Teensy can stall the caller when the USB transmit
// buffer is full (e.g. a host is connected but not reading). log_printf()
// formats into a local buffer and only writes if the whole message fits in
// the USB buffer right now; otherwise the message is dropped and counted.
//
// Do not call from an ISR.

constexpr size_t LOG_LINE_MAX = 192;

void log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Number of messages dropped because the USB buffer was full.
uint32_t log_dropped();
