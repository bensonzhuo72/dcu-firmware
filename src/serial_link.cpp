#include "serial_link.h"

#include "log.h"

namespace {

HardwareSerialIMXRT &port = Serial5;

// Extra buffer space on top of the core's defaults so bursts don't overflow
// between loop passes and writes don't block.
uint8_t g_rx_extra[256];
uint8_t g_tx_extra[256];

char   g_line[LINK_LINE_MAX];
size_t g_line_len = 0;
bool   g_line_overflow = false;

// Bytes handled per poll, so one burst can't starve the rest of the loop.
constexpr int MAX_BYTES_PER_POLL = 256;

}  // namespace

void serial_link_init() {
  port.addMemoryForRead(g_rx_extra, sizeof(g_rx_extra));
  port.addMemoryForWrite(g_tx_extra, sizeof(g_tx_extra));
  port.begin(TEENSY_SERIAL_BAUD);
}

void serial_link_poll() {
  for (int budget = MAX_BYTES_PER_POLL; budget > 0 && port.available(); budget--) {
    char c = static_cast<char>(port.read());

    if (c == '\r') continue;
    if (c == '\n') {
      if (g_line_overflow) {
        log_printf("[LINK] line too long, dropped\r\n");
      } else {
        g_line[g_line_len] = '\0';
        on_link_line(g_line, g_line_len);
      }
      g_line_len = 0;
      g_line_overflow = false;
      continue;
    }

    if (g_line_len < LINK_LINE_MAX - 1) {
      g_line[g_line_len++] = c;
    } else {
      g_line_overflow = true;
    }
  }
}

bool serial_link_send(const uint8_t *data, size_t len) {
  if (port.availableForWrite() < static_cast<int>(len)) return false;
  port.write(data, len);
  return true;
}
