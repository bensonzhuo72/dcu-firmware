#pragma once

#include <Arduino.h>
#include <time.h>

// IEEE 1588 PTP slave using lib/t41-ptp (layer 3: UDP multicast 224.0.1.129,
// ports 319/320, end-to-end delay). The DCU is never the grandmaster; it
// disciplines the Teensy's Ethernet 1588 hardware timer to the master.
//
// PTP starts automatically once the Ethernet link is up and an IP address
// is assigned, and restarts after each link drop. ptp_poll()
// handles at most one event and one general message per call.

// Lock count (consecutive in-tolerance sync updates) required to report
// the clock as locked. Matches the t41-ptp slave example.
constexpr int PTP_LOCK_THRESHOLD = 5;

// Call after network_init().
void ptp_init();

// Call from loop(). Returns true when the locked state changes.
bool ptp_poll();

bool ptp_locked();

// Last measured offset from the master and path delay, in nanoseconds.
int64_t ptp_offset_ns();
int64_t ptp_delay_ns();

// Reads the synchronized 1588 hardware clock. Returns false if the timer
// isn't running. Only meaningful to other devices while ptp_locked().
bool ptp_now(timespec &t);
