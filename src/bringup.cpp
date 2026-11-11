#include "bringup.h"

#include <stdarg.h>

#include "can_test.h"
#include "io.h"
#include "log.h"
#include "network.h"
#include "serial_link.h"

using namespace qindesign::network;

namespace {

uint32_t g_udp_rx = 0;
uint32_t g_udp_tx = 0;
uint32_t g_udp_tx_fail = 0;
uint32_t g_seq = 0;

elapsedMillis g_report_timer;
elapsedMillis g_blink_timer;
bool g_led_on = false;

// TSAB push-button toggle. Pull-up input, so pressed = LOW.
constexpr uint32_t TSAB_DEBOUNCE_MS = 20;
bool g_tsab_raw = HIGH;     // last raw reading
bool g_tsab_stable = HIGH;  // debounced level
elapsedMillis g_tsab_since_change;
bool g_tsab_on = false;     // toggled on each press
uint32_t g_tsab_presses = 0;

void send_heartbeat() {
  if (!Ethernet.linkState() || Ethernet.localIP() == INADDR_NONE) return;
  char buf[64];
  int n = snprintf(buf, sizeof(buf), "DCU heartbeat seq=%lu uptime=%lums",
                   g_seq, millis());
  if (network_send(IPAddress(255, 255, 255, 255), BRINGUP_UDP_HEARTBEAT_PORT,
                   reinterpret_cast<const uint8_t *>(buf), n)) {
    g_udp_tx++;
  } else {
    g_udp_tx_fail++;
  }
}

// Flips g_tsab_on once per debounced press (HIGH -> LOW edge).
void poll_tsab() {
  bool raw = tsab_read();
  if (raw != g_tsab_raw) {
    g_tsab_raw = raw;
    g_tsab_since_change = 0;
    return;
  }
  if (g_tsab_since_change < TSAB_DEBOUNCE_MS || raw == g_tsab_stable) return;

  g_tsab_stable = raw;
  if (raw == LOW) {
    g_tsab_on = !g_tsab_on;
    g_tsab_presses++;
    bringup_printf("[TEST] TSAB pressed -> %s (presses=%lu)\r\n",
                   g_tsab_on ? "ON" : "OFF", g_tsab_presses);
  }
}

void report() {
  IPAddress ip = Ethernet.localIP();
  bringup_printf("[TEST] t=%lus mission=%s DSMS=%d TSAB=%d(%s) BP3V3=%.2fV\r\n",
                 millis() / 1000, mission_name(mission_selected()),
                 dsms_read(), tsab_read(), g_tsab_on ? "ON" : "OFF",
                 bp3v3_read_volts());
  bringup_printf("[TEST] ETH link=%s ip=%u.%u.%u.%u udp rx=%lu tx=%lu fail=%lu "
                 "| CPU %.1fC\r\n",
                 Ethernet.linkState() ? "up" : "down", ip[0], ip[1], ip[2], ip[3],
                 g_udp_rx, g_udp_tx, g_udp_tx_fail, tempmonGetTemp());
}

}  // namespace

void bringup_printf(const char *fmt, ...) {
  char buf[LOG_LINE_MAX];
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  if (n <= 0 || !log_allowed(buf)) return;
  size_t len = min(static_cast<size_t>(n), sizeof(buf) - 1);

  log_printf("%s", buf);
  serial_link_send(reinterpret_cast<const uint8_t *>(buf), len);
}

void bringup_init() {
  // Start from the current TSAB level so a held button at boot isn't a press.
  g_tsab_raw = g_tsab_stable = tsab_read();

  bringup_printf("[TEST] bring-up test running: LEDs flash, UDP echo on %u, "
                 "heartbeat to port %u\r\n",
                 NET_UDP_PORT, BRINGUP_UDP_HEARTBEAT_PORT);

  // Trigger outputs driven high for testing. io_init() starts them low.
  trig1_write(HIGH);
  trig2_write(HIGH);
  bringup_printf("[TEST] TRIG_1_CTRL and TRIG_2_CTRL driven HIGH\r\n");

  // DSR relay on for testing. io_init() starts it low (off).
  dsr_ctrl_write(HIGH);
  bringup_printf("[TEST] DSR_CTRL driven HIGH (relay on); "
                 "precharge/remote held low\r\n");

  can_test_init();
}

void bringup_poll() {
  poll_tsab();

  // Status LEDs flash together: on 500 ms, off 500 ms.
  if (g_blink_timer >= BRINGUP_PERIOD_MS / 2) {
    g_blink_timer -= BRINGUP_PERIOD_MS / 2;
    g_led_on = !g_led_on;
    led_set_blue(g_led_on ? LED_PWM_MAX : 0);
    led_set_yellow(g_led_on ? LED_PWM_MAX : 0);
  }

  can_test_poll();

  if (g_report_timer >= BRINGUP_PERIOD_MS) {
    g_report_timer -= BRINGUP_PERIOD_MS;
    g_seq++;
    send_heartbeat();
    report();
  }
}

void bringup_on_can(Bus bus, uint32_t id, bool extended,
                    const uint8_t *data, uint8_t len) {
  can_test_on_frame(bus, id, extended, data, len);
}

void bringup_on_udp(const uint8_t *data, size_t len,
                    const IPAddress &from_ip, uint16_t from_port) {
  g_udp_rx++;
  // Echo back so the sender can confirm the round trip.
  if (network_send(from_ip, from_port, data, len)) {
    g_udp_tx++;
  } else {
    g_udp_tx_fail++;
  }
}
