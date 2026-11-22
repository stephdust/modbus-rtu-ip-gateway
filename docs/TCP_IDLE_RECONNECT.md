# Persistent TCP Client: independent connection supervision (1.3.8)

## Why this change was made

Before 1.3.8, Client-mode TCP reconnect was mostly **request-driven**. The transaction state machine handled a failed/reused TCP socket, but when no RTU frames arrived, the idle `loop()` returned without checking/reopening the persistent connection. This could appear as a permanent disconnect overnight and was easy to confuse with whether the Web frame diagnostics page was open. Polling `/frames` does **not** control the TCP socket.

## Behavior

- Only **Mode = CLIENT**, **Transport = TCP**, **Persistent TCP = enabled** is supervised. UDP, TCP Server and one-shot TCP retain their previous semantics.
- The supervisor checks the TCP socket roughly **once a second**, only with an idle transaction and no pending RTU bytes.
- TCP keepalive: **15 s idle**, **5 s interval**, **3 probes** if supported. This is a detection mechanism, not a Modbus RTU read/poll.
- TCP reconnect after a detected failure uses a requested **250 ms connect timeout** and progressive **2, 4, 8, 16, 30, 30… second** delays after failed attempts. The first idle connection is attempted on boot once Wi-Fi is available; the connection is supervised even if the RTU listener is stopped in the Web UI.
- A Wi-Fi disconnect closes a stale TCP socket; a later association allows automatic reconnection. A host/port/transport/persistence configuration change similarly closes the previous socket. Configuration changes during an in-flight Modbus transaction return HTTP 409 instead of destroying the ongoing reply.
- **FC03/FC04 retry at most once on a failed reused socket remains unchanged**. No automatic replay of FC05/FC06/FC15/FC16 or other writes.
- No Modbus heartbeat or additional Modbus traffic is sent. A TCP-connected but application-stalled remote Modbus server may still fail request timeouts. DNS resolution and Arduino network setup might take longer than the requested connection timeout.

## Diagnostics

`GET /connections` returns the original `connected`, `mode`, `transport`, `remote` fields plus:

| Field | Meaning |
|---|---|
| `persistent_supervisor` | Whether this mode is eligible for background maintenance |
| `keepalive_configured` | Whether the socket options succeeded for the current TCP socket |
| `reconnect_attempts` | Number of idle TCP connection attempts since boot |
| `reconnect_successes` | Successful idle attempts |
| `reconnect_failures` | Failed idle attempts |
| `idle_disconnects` | Lost or closed sockets recognized by idle supervisor |
| `keepalive_setup_failures` | Socket option failures (keepalive detection may be slower) |
| `next_retry_ms` | Approximate remaining wait before the next retry |

Counters are in RAM and reset on reboot. These are **TCP connection** diagnostics, not Modbus server-aliveness measurements. Web UI polling merely reads them; it is not needed to update them.

## Reproduce and verify the overnight issue

1. Flash **the firmware built from this branch**. Confirm Version 1.3.8, TCP Client + Persistent enabled and the correct server IP and port.
2. Start the real TCP server. Keep **no browser window open** for the gateway, and pause the upstream RTU master so no RTU frames arrive.
3. On first inspection of `/connections`, expect `connected:true` and `persistent_supervisor:true` (after the startup connection). Inspect `keepalive_configured`.
4. Stop the Modbus TCP server or power down its host **without touching the Web UI**; after keepalive/FIN detection and backoff, `connected` should become false and `reconnect_attempts`/`reconnect_failures` should increase.
5. Restart the TCP server; the socket should become connected again without any RTU request. Check `reconnect_successes`.
6. Repeat with the RTU listener stopped in the Web UI. TCP supervision must remain active; no Modbus RTU request should be sent.
7. Repeat with a Wi-Fi interruption. A recovered Wi-Fi association should restore the TCP connection without rebooting.
8. With Modbus RTU traffic enabled, measure latency and ensure no reconnect occurs during an active transaction or partial RTU frame. Test both a stable read and a deliberately dropped connection.
9. Verify no automatic replay of writes (especially FC06/FC16). Verify TCP Server, UDP Client/Server and nonpersistent TCP behavior unchanged.
10. Leave the gateway overnight with TCP server activity and record periodic `/connections` snapshots, ESPHome uptime, memory and frame capture to detect an unrelated crash or Wi-Fi outage.

## Host policy test

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -Icomponents/modbus_gateway \
  tests/test_tcp_connection_policy.cpp -o /tmp/test_tcp_connection_policy
/tmp/test_tcp_connection_policy
```

This test validates only scheduling policy (mode gating, backoff, millis rollover). **It does not emulate lwIP, Wi-Fi, the remote Modbus TCP server, or the ESP32-S3 hardware.** Build/flash and a sustained network-soak test are required before declaring this issue resolved on the physical device.

## Station Wi-Fi vs ESPHome software state (diagnostic 2026-11-22)

The earlier idle supervisor used `wifi::global_wifi_component->is_connected()`
as the sole indicator of connectivity. A false reading could close an
otherwise viable TCP socket. Reading Web logs proves only that the gateway
is reachable **at the time of the request**; it does not prove that STA was
connected when an older frame was captured, and the fallback AP may also
serve a Web UI.

In the experimental STA link fix, decisions are made using ESP-IDF:
`esp_wifi_sta_get_ap_info()` (actual association) AND
`esp_netif_get_ip_info()` on the **WIFI_STA_DEF** interface (IPv4).
An AP-only address must never count as a routed station address. ESPHome
`is_connected()` is still reported, but it cannot alone close a TCP socket.

The `/connections` JSON endpoint exposes independent signals:
- `wifi_esphome_connected`: ESPHome software connection flag
- `wifi_sta_associated`: Wi-Fi driver's STA association
- `wifi_sta_has_ipv4`: nonzero STA interface IPv4 (DHCP or static)
- `wifi_sta_ready`: both driver association and STA IPv4
- `wifi_sta_ip`: actual STA address or `unavailable`
- `tcp_socket_connected`: state of the Modbus TCP client socket
- `wifi_status_disagreements`: count of observed state disagreement edges
- `tcp_connect_skipped_no_sta`: foreground/manual connects skipped without STA
- `tcp_sta_link_losses`: confirmed STA/IP losses seen by the idle supervisor

Each transition mismatch may emit one `NET STATE` diagnostic in
`/frames.txt` (rate-limited to one message every 30 s). The supervisor
checks at most once per second and the foreground transaction also observes
mismatches, so 500 ms RTU polling cannot completely hide them.

If STA association/IPv4 is absent, the gateway does not call blocking
`WiFiClient::connect()` on every RTU request. It returns the configured
silent/exception Modbus failure, without any automatic replay of writes.
The first `TCP OFFLINE` event and then one every 30 skipped foreground
connects provide diagnostics without adding more log spam.

**Important limitation:** when the STA is ready but Flow.io's port 502
is deliberately closed for OTA, `connect()` may still block until the
configured TCP timeout (for example ~3 s). This patch does not change
network socket timeouts or SWR response cache semantics. Those are a
separate performance issue and should not be conflated with a Wi-Fi drop.

### Tests before merging

Run:
```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  -Icomponents/modbus_gateway tests/test_tcp_connection_policy.cpp \
  -o /tmp/test_tcp_connection_policy
/tmp/test_tcp_connection_policy
node tests/check_sta_link_source.mjs
```

Compile the ESPHome YAML against the branch/ref via
`external_components.source: github://stephdust/modbus-rtu-ip-gateway@<commit>`
in Home Assistant before flashing in production. Test normal STA,
temporary loss of DHCP/association, AP-only mode, Flow.io OTA port
closure/reopening, and continuous FC03/FC04 reads. Compare both
`/connections` and `/frames.txt`; do not use the Web UI's reachability
by itself to declare the station connected.
