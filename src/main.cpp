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

#include "can_bus.h"
#include "io.h"
#include "log.h"
#include "network.h"
#include "serial_link.h"

namespace {

constexpr uint32_t STATUS_PERIOD_MS = 5000;

elapsedMillis g_status_timer;
uint32_t g_loop_max_us = 0;

void report_status() {
  log_printf("[SYS] mission=%s loop_max=%luus log_dropped=%lu\r\n",
             mission_name(mission_selected()), g_loop_max_us, log_dropped());
  g_loop_max_us = 0;
}

}  // namespace

// ---- Application handlers (main-loop context) --------------------------------

void on_udp_packet(const uint8_t *data, size_t len,
                   const IPAddress &from_ip, uint16_t from_port) {
  log_printf("[ETH] %u bytes from %u.%u.%u.%u:%u\r\n", len,
             from_ip[0], from_ip[1], from_ip[2], from_ip[3], from_port);
  // TODO: parse commands (e.g. scheduling TRIG_1/TRIG_2).
  (void)data;
}

void on_can_frame(Bus bus, uint32_t id, bool extended,
                  const uint8_t *data, uint8_t len) {
  log_printf("[%s] id=0x%0*lX len=%u\r\n", bus == Bus::CAN0 ? "CAN0" : "CAN1",
             extended ? 8 : 3, id, len);
  // TODO: decode frames.
  (void)data;
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
}

void loop() {
  uint32_t start_us = micros();

  if (mission_update()) {
    log_printf("[IO] mission selected: %s\r\n", mission_name(mission_selected()));
  }

  can_poll();
  network_poll();
  serial_link_poll();

  if (g_status_timer >= STATUS_PERIOD_MS) {
    g_status_timer -= STATUS_PERIOD_MS;
    report_status();
  }

  g_loop_max_us = max(g_loop_max_us, micros() - start_us);
  // The Ethernet stack runs from yield(), which the core calls after loop().
}
