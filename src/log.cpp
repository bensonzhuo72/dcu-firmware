#include "log.h"

#include <stdarg.h>

namespace {
uint32_t g_dropped = 0;
}

void log_printf(const char *fmt, ...) {
  char buf[LOG_LINE_MAX];

  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (n <= 0) return;
  size_t len = static_cast<size_t>(n);
  if (len >= sizeof(buf)) {
    // Truncated: keep the line ending so the next message starts cleanly.
    len = sizeof(buf) - 1;
    buf[len - 2] = '\r';
    buf[len - 1] = '\n';
  }

  if (Serial.availableForWrite() < static_cast<int>(len)) {
    g_dropped++;
    return;
  }
  Serial.write(buf, len);
}

uint32_t log_dropped() { return g_dropped; }
