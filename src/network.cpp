#include <Arduino.h>

#include "log.h"
#include "network.h"

using namespace qindesign::network;

namespace {

// Packets the stack can hold between loop passes.
constexpr size_t UDP_RX_QUEUE = 8;

EthernetUDP g_udp{UDP_RX_QUEUE};

// Packets handled per poll, so a flood can't starve the rest of the loop.
constexpr int MAX_PACKETS_PER_POLL = 4;

}  // namespace

bool network_init() {
  Ethernet.onLinkState([](bool up) {
    log_printf("[ETH] link %s\r\n", up ? "up" : "down");
  });

  Ethernet.onAddressChanged([]() {
    IPAddress ip = Ethernet.localIP();
    if (ip != INADDR_NONE) {
      log_printf("[ETH] IP %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
    }
  });

  // begin() returns immediately; DHCP completes in the background.
  bool ok = NET_USE_DHCP ? Ethernet.begin()
                         : Ethernet.begin(NET_STATIC_IP, NET_SUBNET, NET_GATEWAY);
  if (!ok) {
    log_printf("[ETH] failed to start\r\n");
    return false;
  }

  if (!g_udp.begin(NET_UDP_PORT)) {
    log_printf("[ETH] UDP bind to %u failed\r\n", NET_UDP_PORT);
    return false;
  }
  return true;
}

void network_poll() {
  for (int i = 0; i < MAX_PACKETS_PER_POLL; i++) {
    int size = g_udp.parsePacket();
    if (size < 0) break;  // no packet (0 is a valid empty packet)
    on_udp_packet(g_udp.data(), static_cast<size_t>(size),
                  g_udp.remoteIP(), g_udp.remotePort());
  }
}

bool network_send(const IPAddress &ip, uint16_t port,
                  const uint8_t *data, size_t len) {
  return g_udp.send(ip, port, data, len);
}
