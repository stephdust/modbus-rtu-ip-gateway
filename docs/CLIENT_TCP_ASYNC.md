# Cooperative TCP Client transaction handling

Initial asynchronous design: **1.1.9**. Idle reconnect extension: **1.3.8 (development)**.

In TCP Client mode, a complete RTU request is validated and forwarded to the configured Modbus TCP server. The request is written once; the response is then received incrementally by polling the socket from the component `loop()` function.

## States

1. **Idle** – collect and validate an RTU request. A valid, cacheable request may be answered immediately from the Client-mode cache.
2. **Wait MBAP** – receive the seven-byte Modbus TCP response header incrementally. Validate Transaction ID, protocol ID, length and Unit ID.
3. **Wait PDU** – receive the number of PDU bytes declared by the MBAP length field.
4. **Complete** – validate the function code, update the optional response cache, and transmit the RTU response with a newly calculated RTU CRC.
5. **Error** – close the affected TCP connection, log the reason, and return the existing Modbus gateway-path exception to the RTU master.

The component does not wait in a blocking TCP receive loop. The WebServer and other ESPHome loop work continue while the remote server is processing the request. Persistent TCP mode is still supported: a live socket is reused. Since v1.3.8, an idle supervisor also detects disconnection and attempts reconnection without waiting for a new RTU transaction or a browser request.

## Important timing limits

- The initial TCP connection and reconnect still use Arduino `WiFiClient::connect()`, which is synchronous.
- Sending the RTU reply still waits for UART transmission to complete before releasing RS-485 driver enable. This is required to avoid truncating the physical RTU frame.
- The manual Web UI transaction test remains synchronous. It is rejected with HTTP 409 while an RTU-originated transaction is active, because both use the same TCP socket.
- A single RTU transaction is active at a time. Additional bytes are buffered up to 2 KiB and processed sequentially.
- The TCP timeout is enforced independently for the MBAP header and PDU phases, preserving the previous timeout behavior.

This change improves ESP32 responsiveness during the remote TCP server response wait; it does not reduce the response time of the remote Modbus server or the physical transmission time of an RTU response.

## 1.3.8 idle connection supervision

The component `loop()` calls the persistent TCP supervisor after receiving/draining the UART, independent of any Web endpoint. It runs only in **TCP Client + Persistent** mode when there is no active RTU/TCP transaction and no buffered partial RTU request. Suspending RTU listening does not suspend the socket supervisor.

While Wi-Fi is up, the supervisor checks the TCP socket at most every second. If the connection disappears, it closes the old socket and attempts a new TCP connection using a requested 250 ms timeout, with a retry interval increasing from 2 to 30 seconds. This limit applies to the socket connection attempt; DNS lookups might still take longer.

New persistent sockets use TCP keepalive with 15-second idle time, 5-second probe interval and 3 probes when lwIP supports the socket options. Keepalive can detect a silent TCP endpoint failure, but does not validate remote Modbus application responsiveness. No synthetic Modbus traffic is generated.

The existing FC03/FC04 one-shot retry on failure of a reused socket is preserved; writes are never replayed automatically. The supervisor never starts a second socket while a transaction is in progress. See [reconnect test plan](TCP_IDLE_RECONNECT.md).

**Limitation:** Arduino `WiFiClient::connect()` is synchronous, including in background maintenance. The requested 250 ms timeout minimizes its impact but does not guarantee an absolutely nonblocking loop. Hardware tests should include rapid RTU master polling during a TCP server outage.
