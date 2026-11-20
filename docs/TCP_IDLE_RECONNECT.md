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
