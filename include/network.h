#pragma once

#include <QNEthernet.h>

// Ethernet over the Teensy 4.1's onboard PHY, using QNEthernet.
//
// The Ethernet RX interrupt only hands frames to the stack; the stack runs
// from yield() after every loop(), and network_poll() drains received UDP
// packets in the main loop. Nothing here waits for link or DHCP.

// Set to false to use the static address below instead of DHCP.
constexpr bool NET_USE_DHCP = true;

const IPAddress NET_STATIC_IP{192, 168, 1, 50};
const IPAddress NET_SUBNET{255, 255, 255, 0};
const IPAddress NET_GATEWAY{192, 168, 1, 1};

// UDP port the DCU listens on.
constexpr uint16_t NET_UDP_PORT = 5000;

bool network_init();
void network_poll();

// Sends a UDP packet. Returns false if it could not be queued.
bool network_send(const IPAddress &ip, uint16_t port,
                  const uint8_t *data, size_t len);

// Implemented by the application. Called from the main loop for each
// received UDP packet. `data` is only valid during the call.
void on_udp_packet(const uint8_t *data, size_t len,
                   const IPAddress &from_ip, uint16_t from_port);
