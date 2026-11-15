# TCP/UDP transport integration plan (work in progress)

This document describes the intended implementation, **not an implemented or tested UDP feature**. The working TCP firmware on `main` remains unchanged.

## Baseline verified from source

- ESPHome custom component: `components/modbus_gateway/modbus_gateway.h` and `__init__.py`.
- Arduino `WiFiClient`, `WiFiServer`, `WebServer` and `Preferences`.
- Client: RTU requests forwarded to TCP with a cooperative, polled transaction state machine.
- Server: at most four persistent TCP clients, serialized RS-485 transactions.
- Separate read caches for Client and Server modes, frame logs, configurable timeout policy, manual/automatic RTU address handling.
- Current mode field is `tcp_mode_` (`CLIENT` or `SERVER`); network transport selection is not implemented.

## Protocol contract

Both TCP and UDP carry the Modbus TCP ADU: 7-byte MBAP header (transaction ID, protocol ID=0, length, unit ID) followed by a Modbus PDU. UDP transports **one complete ADU per datagram**, with no extra RTU CRC. Validate MBAP protocol ID, declared length, datagram size and PDU bounds before dispatch. RTU messages retain their CRC on the serial wire.

Client mode (RTU master -> gateway -> IP server):
- Preserve incoming RTU slave address as MBAP Unit ID.
- Choose TCP or UDP independently of the gateway's Client/Server role.
- UDP: send a datagram to the configured host/port; wait cooperatively for a matching response (transaction ID, source address/port, protocol ID, Unit ID and valid length); ignore unrelated/late datagrams.
- On timeout apply the existing Standard/Silent policy. Define configurable UDP retries (default zero to avoid unexpected duplicate writes); never retry writes without explicit opt-in.

Server mode (IP master -> gateway -> RTU slave):
- TCP: retain existing persistent-client behavior.
- UDP: accept complete request datagrams on the configured listening port, retain source IP/port and transaction ID, serialize RTU bus access, send reply to the originating endpoint.
- In automatic mode map incoming MBAP Unit ID to RTU address; in manual mode use configured RTU address but preserve original MBAP Unit ID in response.
- Avoid processing duplicate UDP requests concurrently; do not replay writes automatically. Establish clear bounded queue and overflow behavior.

## Implementation sequence

1. Extract transport-neutral request validation, RTU scheduling, response generation and cache decisions from existing TCP paths without changing TCP behavior.
2. Introduce persisted `transport=TCP|UDP` selection in the bilingual web UI; migrate existing NVS settings to TCP by default. Keep `CLIENT|SERVER` as a separate mode.
3. Add `WiFiUDP` and cooperative UDP Client state handling, with bounded packet buffers and transaction matching.
4. Add UDP Server listener and source-aware response routing, respecting sequential RS-485 access.
5. Extend FR/EN diagnostics to identify transport and endpoint; retain CRC checks and existing manual frame testing.
6. Add focused protocol tests (malformed MBAP, length mismatch, out-of-order and duplicate datagrams, timeouts, cache invalidation, write retries, mode/transport changes).
7. Compile against the target ESPHome/Arduino toolchain, then validate on both Waveshare boards before describing UDP as supported.

## Compatibility and security

- Keep default TCP port 502, HTTP UI port 80 and ESPHome native UI port 8080.
- Preserve GPIO17 TX, GPIO18 RX, GPIO21 DE/RE, serial defaults and existing TCP performance behavior.
- Do not change the `main` branch until firmware compilation and hardware validation are complete.
- UDP has no connection, encryption or authentication. Deploy on a trusted LAN/VLAN only.
