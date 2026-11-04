# Vendored: t41-ptp

- Upstream: <https://github.com/IMS-AS-LUH/t41-ptp>
- Commit: 541d9f3a2af92a6547a2be165e1c924e13186681 (2025-08-28)
- License: MIT (see LICENSE)
- Requires the QNEthernet IEEE 1588 fork pinned in platformio.ini:
  <https://github.com/HedgeHawk/QNEthernet/tree/ieee1588-2-fix>
  (commit 6edbded44ee610e6e08c061dff594065cde321d2)

Only `src/`, `LICENSE`, `library.properties` and `README.md` were copied.

## Local changes

`src/ptp/ptp-base.cpp`: the waits for the hardware TX timestamp in
`syncMessage()` and `delayRequestMessage()` looped forever. If the frame is
never sent (link drop, no buffer), that hung the firmware. They now go
through `waitTxTimestamp()`, which gives up after 1 ms and drops that one
exchange. Search for "DCU patch".

`src/ptp/ptp-base.cpp`, `parsePTPMessage()`: added parentheses to
`slave && (messageType==3 || messageType==9)` to fix a -Wparentheses
warning. Upstream parsed as `(slave && type==3) || type==9`; the result is
identical for a slave, and a master no longer handles slave-only responses.

`src/ptp/l2ptp.cpp`, `sendPTPMessage()`: cast the unused `sent` result to
`void` to fix a -Wunused-variable warning. No behavior change.

Re-apply this change if the library is updated.
