#include "can_test.h"

#include "bringup.h"  // bringup_printf: USB + UART

namespace {

const char *const NAME[2] = {"CAN0", "CAN1"};
const char *const PINS[2] = {"pins 22/23", "pins 0/1"};
const Bus BUS[2] = {Bus::CAN0, Bus::CAN1};

struct Totals {
  uint32_t pings = 0, echoed = 0, no_echo = 0, tx_fail = 0;
  uint32_t hb = 0, hb_gaps = 0, late = 0, other = 0;
};

struct BusTest {
  // The period in progress.
  bool sent = false;      // ping queued
  bool tx_fail = false;   // ping could not be queued
  bool echoed = false;
  uint32_t sent_us = 0;
  uint32_t rtt_us = 0;
  uint32_t hb = 0;
  bool hb_gap = false;

  bool hb_seen = false;
  uint32_t hb_last_seq = 0;
  const char *last_hint = "";  // "" = nothing printed yet
  Totals total;
};

BusTest g_bus[2];
uint32_t g_seq = 0;  // ping sequence, shared by both buses
elapsedMillis g_timer;

uint32_t get_u32(const uint8_t *p) {
  uint32_t v;
  memcpy(&v, p, 4);
  return v;
}

const char *state_name(CanState s) {
  switch (s) {
    case CanState::Active:  return "OK";
    case CanState::Passive: return "ERR-PASSIVE";
    case CanState::BusOff:  return "BUS-OFF";
  }
  return "?";
}

// Most likely cause, from the strongest evidence down. nullptr = healthy.
const char *diagnose(const BusTest &b, const CanErrors &e) {
  if (b.echoed) return nullptr;
  if (e.state == CanState::BusOff || e.bit0 || e.bit1)
    return "DCU can't see its own frames: check transceiver power and standby pin, "
           "TXD/RXD wiring, CANH-CANL short";
  if (e.stuff || e.form || e.crc)
    return "corrupted frames: check both ends at 500k, ~60 ohm CANH-CANL, "
           "CANH/CANL not swapped";
  if (e.ack || e.state == CanState::Passive)
    return "nobody ACKs our frames: partner powered and running? "
           "same CANH/CANL pair? common GND?";
  if (b.hb > 0)
    return "partner heard (heartbeat) but no echo: check partner echoes 0x001/0x002";
  return "no echo, no heartbeat, no bus errors: is the partner sketch running?";
}

void summarize(int i) {
  BusTest &b = g_bus[i];
  CanErrors e = can_errors(BUS[i]);

  char result[24];
  if (b.tx_fail)     snprintf(result, sizeof(result), "TX-FULL");
  else if (b.echoed) snprintf(result, sizeof(result), "PASS    echo %luus", b.rtt_us);
  else               snprintf(result, sizeof(result), "NO-ECHO");

  bringup_printf("[%s] #%-4lu %-21s hb %lu%s | %-11s TEC=%-3u REC=%-3u%s%s%s%s%s%s\r\n",
                 NAME[i], g_seq, result, b.hb, b.hb_gap ? " (gap)" : "",
                 state_name(e.state), e.tec, e.rec,
                 e.ack ? " ACK" : "", e.bit0 ? " BIT0" : "", e.bit1 ? " BIT1" : "",
                 e.stuff ? " STUFF" : "", e.form ? " FORM" : "", e.crc ? " CRC" : "");

  const char *hint = diagnose(b, e);
  if (hint != b.last_hint) {
    bringup_printf("[%s]   -> %s\r\n", NAME[i], hint ? hint : "link OK");
    b.last_hint = hint;
  }

  Totals &t = b.total;
  if (b.sent) t.pings++;
  if (b.tx_fail) t.tx_fail++;
  if (b.echoed) t.echoed++;
  if (b.sent && !b.echoed) t.no_echo++;
}

void print_totals(int i) {
  const Totals &t = g_bus[i].total;
  bringup_printf("[%s] totals: pings=%lu echoed=%lu no_echo=%lu tx_full=%lu "
                 "hb=%lu hb_gaps=%lu late=%lu other=%lu\r\n",
                 NAME[i], t.pings, t.echoed, t.no_echo, t.tx_fail, t.hb, t.hb_gaps,
                 t.late, t.other);
}

// Starts a new period: clears the per-period state and sends the ping.
void start_period(int i) {
  BusTest &b = g_bus[i];
  b.sent = b.tx_fail = b.echoed = b.hb_gap = false;
  b.hb = 0;

  uint32_t t = millis();
  uint8_t data[8];
  memcpy(&data[0], &g_seq, 4);
  memcpy(&data[4], &t, 4);

  b.sent_us = micros();
  if (can_send(BUS[i], CAN_TEST_PING_ID[i], data, sizeof(data))) {
    b.sent = true;
  } else {
    b.tx_fail = true;
  }
}

void on_echo(int rx, uint32_t id, const uint8_t *data, uint8_t len) {
  int origin = (id == CAN_TEST_ECHO_ID[0]) ? 0 : 1;
  BusTest &b = g_bus[rx];
  uint32_t seq = len >= 4 ? get_u32(data) : 0;

  if (origin != rx) {
    b.total.other++;
    bringup_printf("[%s] echo 0x%03lX belongs to %s's ping: are CAN0 and CAN1 "
                   "wired together?\r\n", NAME[rx], id, NAME[origin]);
  } else if (b.sent && !b.echoed && seq == g_seq) {
    b.echoed = true;
    b.rtt_us = micros() - b.sent_us;
  } else {
    // Echo of an older ping, e.g. queued frames going out after a bus-off.
    b.total.late++;
    bringup_printf("[%s] late echo seq=%lu (current #%lu)\r\n", NAME[rx], seq, g_seq);
  }
}

void on_heartbeat(int rx, const uint8_t *data, uint8_t len) {
  BusTest &b = g_bus[rx];
  uint32_t seq = len >= 4 ? get_u32(data) : 0;
  if (b.hb_seen && seq != b.hb_last_seq + 1) {
    b.hb_gap = true;
    b.total.hb_gaps++;
  }
  b.hb_seen = true;
  b.hb_last_seq = seq;
  b.hb++;
  b.total.hb++;
}

}  // namespace

void can_test_init() {
  bringup_printf("[CAN] link test: 500 kbit/s, ping every %lu ms, partner sketch "
                 "tools/can_partner\r\n", CAN_TEST_PERIOD_MS);
  for (int i = 0; i < 2; i++) {
    if (CAN_TEST_BUS_ENABLED[i]) {
      bringup_printf("[%s] ON  (%s): ping 0x%03lX, expect echo 0x%03lX + heartbeat\r\n",
                     NAME[i], PINS[i], CAN_TEST_PING_ID[i], CAN_TEST_ECHO_ID[i]);
    } else {
      bringup_printf("[%s] off (%s)\r\n", NAME[i], PINS[i]);
    }
  }
  bringup_printf("[CAN] each line: #ping result echo-time | heartbeats received | "
                 "controller state, error counters, error types seen\r\n");
}

void can_test_poll() {
  if (g_timer < CAN_TEST_PERIOD_MS) return;
  g_timer -= CAN_TEST_PERIOD_MS;

  // Report the period that just ended (none before the first ping).
  if (g_seq > 0) {
    for (int i = 0; i < 2; i++) {
      if (CAN_TEST_BUS_ENABLED[i]) summarize(i);
    }
    if (g_seq % CAN_TEST_TOTALS_EVERY == 0) {
      for (int i = 0; i < 2; i++) {
        if (CAN_TEST_BUS_ENABLED[i]) print_totals(i);
      }
    }
  }

  g_seq++;
  for (int i = 0; i < 2; i++) {
    if (CAN_TEST_BUS_ENABLED[i]) start_period(i);
  }
}

void can_test_on_frame(Bus bus, uint32_t id, bool extended,
                       const uint8_t *data, uint8_t len) {
  int rx = (bus == Bus::CAN0) ? 0 : 1;
  if (!CAN_TEST_BUS_ENABLED[rx]) return;

  if (!extended && (id == CAN_TEST_ECHO_ID[0] || id == CAN_TEST_ECHO_ID[1])) {
    on_echo(rx, id, data, len);
  } else if (!extended && (id == CAN_TEST_HB_ID[0] || id == CAN_TEST_HB_ID[1])) {
    on_heartbeat(rx, data, len);
  } else {
    g_bus[rx].total.other++;
    char hex[3 * 8 + 1] = "";
    for (uint8_t i = 0; i < len && i < 8; i++) snprintf(&hex[3 * i], 4, " %02X", data[i]);
    bringup_printf("[%s] unexpected frame id=0x%0*lX len=%u data:%s\r\n", NAME[rx],
                   extended ? 8 : 3, id, len, hex);
  }
}
