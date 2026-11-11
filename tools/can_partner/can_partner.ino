// CAN test partner for the DCU bring-up firmware (Teensy 4.0 / 4.1, Arduino IDE).
//
// Wiring: Teensy pin 22 (CTX1) -> transceiver TXD, pin 23 (CRX1) -> transceiver RXD.
//         CANH-CANH and CANL-CANL to the DCU, common GND, 120 ohm at each end.
//
// - Echoes DCU pings (ID 0x001 or 0x002) back with ID + 0x100 (0x101 / 0x102).
// - Sends a heartbeat (ID 0x201) every 2 s: [0..3] count, [4..7] uptime ms.
// - Prints everything it sends and receives, plus the CAN error state.
// Serial Monitor: 115200.

#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can;  // pins 22 / 23

uint32_t count = 0;
elapsedMillis timer;

void setup() {
  Serial.begin(115200);
  can.begin();
  can.setBaudRate(500000);
  Serial.println("CAN partner on pins 22/23, 500 kbit/s");
}

void loop() {
  CAN_message_t msg;

  // Receive: print it, and echo pings back.
  while (can.read(msg)) {
    Serial.printf("RX id=0x%03lX len=%u data:", msg.id, msg.len);
    for (int i = 0; i < msg.len; i++) Serial.printf(" %02X", msg.buf[i]);
    Serial.println();

    if (msg.id == 0x001 || msg.id == 0x002) {
      msg.id += 0x100;
      can.write(msg);
      Serial.printf("TX echo id=0x%03lX\n", msg.id);
    }
  }

  // Every 2 s: send a heartbeat and print the error state.
  if (timer >= 2000) {
    timer = 0;
    count++;

    CAN_message_t hb;
    hb.id = 0x201;
    hb.len = 8;
    uint32_t up = millis();
    memcpy(&hb.buf[0], &count, 4);
    memcpy(&hb.buf[4], &up, 4);
    bool ok = can.write(hb) > 0;
    Serial.printf("TX heartbeat id=0x201 count=%lu %s\n", count, ok ? "" : "FAILED (queue full)");

    // CAN1 error registers: ECR = TX/RX error counters, ESR1 bits 4-5 = state.
    uint32_t ecr = *(volatile uint32_t *)0x401D001C;
    uint32_t esr = *(volatile uint32_t *)0x401D0020;
    uint32_t state = (esr >> 4) & 3;
    Serial.printf("  errors: TEC=%lu REC=%lu %s\n", ecr & 0xFF, (ecr >> 8) & 0xFF,
                  state == 0 ? "OK" : state == 1 ? "ERROR-PASSIVE (nobody ACKing)" : "BUS-OFF");
  }
}
