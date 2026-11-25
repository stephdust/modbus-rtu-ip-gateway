# Modbus TCP socket and ESPHome STA diagnostics

Originally developed on the diagnostic branch, these probes are now part of
**`main` (`1.3.16-rc2`)**. They preserve the Modbus/RTU scheduler,
cache, backoff and TCP close/reconnect logic.

## Compile through ESPHome

Set `external_components` to
`github://stephdust/modbus-rtu-ip-gateway@main` with `refresh: 0s`.
The GitHub example has been updated, but an existing Home Assistant ESPHome
configuration does **not** change automatically. Pin a commit SHA rather
than moving `main` to reproduce a specific build.

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
- In instrumented Flow.io versions, `[MODBUS-RX]` identified the `peer=IP:port`,
  socket source, `errno` and the age of the **recorded error at publication**,
  not the socket connection age. The cleaned Flow.io branch omits this
  detailed log in normal operation.

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
lwIP behavior or ESPHome firmware compilation. Initial hardware testing with `1.3.16-rc2` is complete, but extended
endurance and all transport modes have not yet been qualified.
