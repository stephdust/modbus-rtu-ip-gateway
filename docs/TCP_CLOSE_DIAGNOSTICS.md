# Modbus TCP socket and ESPHome STA diagnostics (test branch)

This is an instrumentation-only patch. It preserves the existing Modbus
transaction/RTU scheduler, cache, retry/backoff and TCP close/reconnect logic.

## Compile through ESPHome

The source `external_components` must point to
`github://stephdust/modbus-rtu-ip-gateway@diag/tcp-close-wifi-reasons-20261125`
while testing, with `refresh: 0s`. This repository's diagnostic branch
contains a matching `my-modbus-gateway.yaml` example; your already installed
ESPHome YAML in Home Assistant does **not** switch branches automatically.
After merging and validating a release, restore the production `@main` ref.

## Capture and compare

Use the Gateway Web UI `/frames.txt` and Flow.io exported runtime logs.
Match:

- Gateway `TCP OPEN`: `origin`, `sid` (gateway-local), `fd`,
  `local_port`, remote Flow.io address and port.
- Gateway `TCP CLOSE`: explicit cause, same `sid` and `local_port`,
  `driver_assoc`, `sta_ipv4`, `esphome_sw`, transaction id and
  whether a background refresh was active.
- Gateway `WIFI STA DOWN`: ESP-IDF `WIFI_EVENT_STA_DISCONNECTED` with
  numeric disconnect reason (from the **driver**, not ESPHome's
  `is_connected()`), last RSSI/BSSID, event count.
- Gateway `WIFI STA IP LOST`: ESP-IDF IP loss event; logs current driver
  association, station IPv4 presence and ESPHome software state. An IP loss
  is not necessarily a loss of radio association.
- Flow.io `[MODBUS-RX]`: `peer=IP:port`, socket source `recv`/`peek`,
  TCP session id, `errno` and the delayed snapshot age.

The Gateway `sid` and Flow.io `sid` are **not the same numeric ID**.
Correlate TCP connections using the Gateway STA IP + `local_port` against
Flow.io `peer=IP:port`, then timestamps. Socket file descriptor numbers are
also local to each ESP32 and not comparable across devices.

The ESPHome software `is_connected()` flag is deliberately recorded but never
used by these diagnostics to close a socket. Flow.io `errno=104`
(`ECONNRESET`) alone does not prove a Wi-Fi fault: use the matching
Gateway `TCP CLOSE` cause and the driver/IP event history.

## Regression tests

```sh
node tests/check_tcp_close_diagnostics.mjs
node tests/check_sta_link_source.mjs
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -Icomponents/modbus_gateway tests/test_tcp_connection_policy.cpp -o /tmp/test_tcp && /tmp/test_tcp
```

These tests validate local policy/source invariants, **not** Wi-Fi hardware,
lwIP behavior or ESPHome firmware compilation. Test on the ESP32-S3 before
merging to `main`.
