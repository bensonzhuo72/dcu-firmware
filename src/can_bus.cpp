#include "can_bus.h"

#include <FlexCAN_T4.h>

namespace {

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can0;  // schematic CAN0
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can1;  // schematic CAN1

// Frames handled per bus per poll, so a busy bus can't starve the loop.
constexpr int MAX_FRAMES_PER_POLL = 16;

void on_can0_rx(const CAN_message_t &msg) {
  on_can_frame(Bus::CAN0, msg.id, msg.flags.extended, msg.buf, msg.len);
}

void on_can1_rx(const CAN_message_t &msg) {
  on_can_frame(Bus::CAN1, msg.id, msg.flags.extended, msg.buf, msg.len);
}

template <typename T>
void bus_init(T &bus, uint32_t baud, void (*handler)(const CAN_message_t &)) {
  bus.begin();
  bus.setBaudRate(baud);
  bus.setMaxMB(16);
  bus.enableFIFO();
  bus.onReceive(handler);
  // FlexCAN_T4 runs the receive callback inside the ISR until events() has
  // been called once. Call it before enabling the interrupt so frames are
  // always queued and handled in the main loop instead.
  bus.events();
  bus.enableFIFOInterrupt();
}

// events() handles one queued RX frame (and services the TX queue) per call
// and returns the number of RX frames still queued in bits 12 and up.
template <typename T>
void bus_poll(T &bus) {
  for (int i = 0; i < MAX_FRAMES_PER_POLL; i++) {
    if ((bus.events() >> 12) == 0) break;
  }
}

}  // namespace

void can_init() {
  bus_init(can0, CAN0_BAUD, on_can0_rx);
  bus_init(can1, CAN1_BAUD, on_can1_rx);
}

void can_poll() {
  bus_poll(can0);
  bus_poll(can1);
}

bool can_send(Bus bus, uint32_t id, const uint8_t *data, uint8_t len,
              bool extended) {
  if (len > 8 || (len > 0 && data == nullptr)) return false;

  CAN_message_t msg;
  msg.id = id;
  msg.flags.extended = extended;
  msg.len = len;
  if (len > 0) memcpy(msg.buf, data, len);

  int result = (bus == Bus::CAN0) ? can0.write(msg) : can1.write(msg);
  return result > 0;
}
