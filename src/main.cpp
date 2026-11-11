// DCU firmware main loop.
//
// Architecture rules:
//  - ISRs only capture data (set a flag, record a timestamp, push into a
//    queue). They never call application code, print or wait.
//  - Each module has an init() and a non-blocking poll(). poll() handles a
//    bounded amount of work per call so no single source can starve the loop.
//  - Received data reaches the application through the on_*() handlers below,
//    which always run in main-loop context.
//  - Nothing in loop() waits: no delay(), no while(!ready), no blocking I/O.
//    Periodic work is scheduled with elapsedMillis.

#include <Arduino.h>

#include "bringup.h"
#include "can_bus.h"
#include "io.h"
#include "log.h"
#include "network.h"
#include "ptp_sync.h"
#include "serial_link.h"

namespace {

constexpr uint32_t STATUS_PERIOD_MS = 5000;

// Bench bring-up test (see bringup.h). Set to false for normal operation.
constexpr bool BRINGUP_ENABLED = true;

elapsedMillis g_status_timer;

// Longest time between consecutive loop() entries. This includes yield(),
// where the Ethernet stack runs, so it catches blocking anywhere.
uint32_t g_loop_last_us = 0;
uint32_t g_loop_max_us = 0;

void report_status() {
  // Offsets can be ~1e18 ns before the first sync, so print as 64-bit.
  log_printf("[SYS] mission=%s ptp=%s offset=%lldns delay=%lldns "
             "loop_max=%luus log_dropped=%lu\r\n",
             mission_name(mission_selected()),
             ptp_locked() ? "LOCKED" : "unlocked",
             static_cast<long long>(ptp_offset_ns()),
             static_cast<long long>(ptp_delay_ns()),
             g_loop_max_us, log_dropped());
  g_loop_max_us = 0;
}

}  // namespace

// ---- Application handlers (main-loop context) --------------------------------

void on_udp_packet(const uint8_t *data, size_t len,
                   const IPAddress &from_ip, uint16_t from_port) {
  log_printf("[ETH] %u bytes from %u.%u.%u.%u:%u\r\n", len,
             from_ip[0], from_ip[1], from_ip[2], from_ip[3], from_port);
  if (BRINGUP_ENABLED) bringup_on_udp(data, len, from_ip, from_port);
  // TODO: parse commands (e.g. scheduling TRIG_1/TRIG_2).
}

void on_can_frame(Bus bus, uint32_t id, bool extended,
                  const uint8_t *data, uint8_t len) {
  if (BRINGUP_ENABLED) {
    // The test module prints every frame itself (USB and UART).
    bringup_on_can(bus, id, extended, data, len);
    return;
  }
  char hex[3 * 8 + 1] = "";
  for (uint8_t i = 0; i < len && i < 8; i++) {
    snprintf(&hex[3 * i], 4, " %02X", data[i]);
  }
  log_printf("[%s] id=0x%0*lX len=%u data:%s\r\n",
             bus == Bus::CAN0 ? "CAN0" : "CAN1", extended ? 8 : 3, id, len, hex);
  // TODO: decode frames.
}

void on_link_line(const char *line, size_t len) {
  log_printf("[LINK] %.*s\r\n", static_cast<int>(len), line);
  // TODO: handle serial link commands.
}

// ---- Setup / loop -------------------------------------------------------------

void setup() {
  Serial.begin(115200);  // USB; does not wait for a host

  io_init();
  serial_link_init();
  can_init();
  network_init();
  ptp_init();
  if (BRINGUP_ENABLED) bringup_init();

  g_loop_last_us = micros();
}

void loop() {
  uint32_t now_us = micros();
  g_loop_max_us = max(g_loop_max_us, now_us - g_loop_last_us);
  g_loop_last_us = now_us;

  if (mission_update()) {
    log_printf("[IO] mission selected: %s\r\n", mission_name(mission_selected()));
  }

  if (ptp_poll()) {
    log_printf("[PTP] %s\r\n", ptp_locked() ? "locked" : "lost lock");
  }

  can_poll();
  network_poll();
  serial_link_poll();
  if (BRINGUP_ENABLED) bringup_poll();

  if (g_status_timer >= STATUS_PERIOD_MS) {
    g_status_timer -= STATUS_PERIOD_MS;
    report_status();
  }

  // The Ethernet stack runs from yield(), which the core calls after loop().
}
