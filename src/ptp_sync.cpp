#include "ptp_sync.h"

#include <QNEthernet.h>
#include <t41-ptp.h>

using qindesign::network::Ethernet;
using qindesign::network::EthernetIEEE1588;

namespace {

constexpr bool kMaster = false;
constexpr bool kSlave  = true;
constexpr bool kP2P    = false;  // end-to-end delay mechanism

l3PTP g_ptp(kMaster, kSlave, kP2P);

bool g_running = false;
bool g_locked  = false;

}  // namespace

void ptp_init() {
  // Start the 1588 hardware timer. Must come after Ethernet.begin().
  EthernetIEEE1588.begin();
}

bool ptp_poll() {
  // (Re)start PTP once the link is up and an IP is assigned. With DHCP the
  // link comes up before the address, and the multicast join needs the
  // interface configured. begin() resets the servo state and only creates
  // the sockets the first time.
  bool ready = Ethernet.linkState() && Ethernet.localIP() != INADDR_NONE;
  if (ready && !g_running) g_ptp.begin();
  g_running = ready;

  if (g_running) g_ptp.update();

  bool locked = g_running && g_ptp.getLockCount() > PTP_LOCK_THRESHOLD;
  if (locked == g_locked) return false;
  g_locked = locked;
  return true;
}

bool ptp_locked() { return g_locked; }

int64_t ptp_offset_ns() { return g_ptp.getOffset(); }
int64_t ptp_delay_ns()  { return g_ptp.getDelay(); }

bool ptp_now(timespec &t) { return EthernetIEEE1588.readTimer(t); }
