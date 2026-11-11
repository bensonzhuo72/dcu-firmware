#include "bringup.h"

#include <stdarg.h>

#include "io.h"
#include "log.h"
#include "network.h"
#include "serial_link.h"

using namespace qindesign::network;

namespace {

struct CanStats {
  uint32_t tx_ok = 0;
  uint32_t tx_fail = 0;  // TX queue full: usually no other node ACKing
  uint32_t rx_id1 = 0;
  uint32_t rx_id2 = 0;
};

CanStats g_can[2];
uint32_t g_udp_rx = 0;
uint32_t g_udp_tx = 0;
uint32_t g_udp_tx_fail = 0;
uint32_t g_seq = 0;

elapsedMillis g_blink_timer;
elapsedMillis g_report_timer;
bool g_led_on = false;

CanStats &stats(Bus bus) { return g_can[bus == Bus::CAN0 ? 0 : 1]; }

// Prints one line to USB (log) and the UART link. Never blocks.
void out_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void out_printf(const char *fmt, ...) {
  char buf[LOG_LINE_MAX];
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  if (n <= 0) return;
  size_t len = min(static_cast<size_t>(n), sizeof(buf) - 1);

  log_printf("%s", buf);
  serial_link_send(reinterpret_cast<const uint8_t *>(buf), len);
}

void send_can(Bus bus, uint32_t id) {
  uint32_t t = millis();
  uint8_t data[8];
  memcpy(&data[0], &g_seq, 4);  // sequence number, little-endian
  memcpy(&data[4], &t, 4);      // uptime ms, little-endian
  CanStats &s = stats(bus);
  if (can_send(bus, id, data, sizeof(data))) {
    s.tx_ok++;
  } else {
    s.tx_fail++;
  }
}

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

void report() {
  IPAddress ip = Ethernet.localIP();
  out_printf("[TEST] t=%lus mission=%s DSMS=%d TSAB=%d BP3V3=%.2fV\r\n",
             millis() / 1000, mission_name(mission_selected()),
             dsms_read(), tsab_read(), bp3v3_read_volts());
  out_printf("[TEST] CAN0 tx=%lu fail=%lu rx(id1/id2)=%lu/%lu | "
             "CAN1 tx=%lu fail=%lu rx(id1/id2)=%lu/%lu\r\n",
             g_can[0].tx_ok, g_can[0].tx_fail, g_can[0].rx_id1, g_can[0].rx_id2,
             g_can[1].tx_ok, g_can[1].tx_fail, g_can[1].rx_id1, g_can[1].rx_id2);
  out_printf("[TEST] ETH link=%s ip=%u.%u.%u.%u udp rx=%lu tx=%lu fail=%lu "
             "| CPU %.1fC\r\n",
             Ethernet.linkState() ? "up" : "down", ip[0], ip[1], ip[2], ip[3],
             g_udp_rx, g_udp_tx, g_udp_tx_fail, tempmonGetTemp());
}

}  // namespace

void bringup_init() {
  out_printf("[TEST] bring-up test running: LEDs blink, CAN0 tx id %lu, "
             "CAN1 tx id %lu, UDP echo on %u, heartbeat to port %u\r\n",
             BRINGUP_CAN0_TX_ID, BRINGUP_CAN1_TX_ID, NET_UDP_PORT,
             BRINGUP_UDP_HEARTBEAT_PORT);
  out_printf("[TEST] control outputs are held low and not exercised\r\n");
}

void bringup_poll() {
  if (g_blink_timer >= BRINGUP_PERIOD_MS / 2) {
    g_blink_timer -= BRINGUP_PERIOD_MS / 2;
    g_led_on = !g_led_on;
    led_set_blue(g_led_on ? LED_PWM_MAX : 0);
    led_set_yellow(g_led_on ? LED_PWM_MAX : 0);
  }

  if (g_report_timer >= BRINGUP_PERIOD_MS) {
    g_report_timer -= BRINGUP_PERIOD_MS;
    g_seq++;
    send_can(Bus::CAN0, BRINGUP_CAN0_TX_ID);
    send_can(Bus::CAN1, BRINGUP_CAN1_TX_ID);
    send_heartbeat();
    report();
  }
}

void bringup_on_can(Bus bus, uint32_t id) {
  CanStats &s = stats(bus);
  if (id == 1) s.rx_id1++;
  if (id == 2) s.rx_id2++;
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
