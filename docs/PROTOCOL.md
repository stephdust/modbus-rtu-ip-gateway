<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 Modbus RTU-TCP Gateway contributors. -->

# Modbus protocol behaviour

## Operating modes

This document's existing TCP sections describe TCP-specific framing and socket behavior. Version **1.2.1** also supports UDP Client and UDP Server, described below; both use Modbus MBAP framing over UDP datagrams. The UI's Client/Server direction and Unit ID settings apply to the selected transport.


### TCP Client — RTU to TCP

1. The gateway receives an RTU request on the UART/RS-485 interface.
2. It checks the RTU CRC and extracts the RTU slave address and PDU.
3. It uses the persistent TCP connection to the configured Modbus TCP host and port (opening or reconnecting the socket when needed).
4. The RTU slave address is copied to the Modbus TCP Unit ID without applying the manual RTU ID override.
5. The matching Modbus TCP response is checked and returned to the RTU master with a newly calculated RTU CRC.

The Client mode can keep one outbound TCP socket open between RTU transactions (enabled by default in the Web UI). When persistent mode is disabled, a connection is opened for each transaction. A disconnected persistent session is detected and re-established before a subsequent request.

### TCP Server — TCP to RTU

1. The gateway accepts up to four TCP client sessions on the configured listening port (502 by default).
2. Each client has its own connection slot and request parser.
3. A complete Modbus TCP ADU is validated (Protocol ID must be zero and the MBAP length must be in the supported range).
4. The request is converted to an RTU frame with a calculated CRC and sent over the shared RS-485 bus.
5. The RTU response is collected and CRC-checked.
6. The response is sent to the same TCP socket using the original Transaction ID and original TCP Unit ID.

The TCP sessions remain open between transactions. An incomplete Modbus TCP request that exceeds the configured timeout causes that client's slot/socket to be reset. A connection above the four-client limit is rejected.

## Remote timeout policy (v1.1.13)

The Web UI provides one **Remote timeout behavior** setting shared by both operating modes. The TCP timeout itself is configurable from 1 to 30 seconds (default 3 seconds). The policy is saved in ESP32 non-volatile preferences and defaults to `Standard` on a fresh installation.

### Standard (default)

- **TCP Client:** if the remote Modbus TCP server does not return a complete response before the response timeout, the gateway sends an RTU exception to the original RTU master. The exception function is `request function | 0x80`, with exception code `0x0B` (Gateway Target Device Failed to Respond).
- **TCP Server:** if the downstream RTU slave does not return a complete response before the RTU response timeout, the gateway sends a Modbus TCP exception `0x0B` to the TCP client that issued the request. The original Transaction ID and TCP Unit ID are retained.

### Silent

For those same remote-response timeouts, the gateway sends **no RTU response** in TCP Client mode and **no TCP response** in TCP Server mode. The upstream RTU master or TCP client must therefore apply its own timeout. In Server mode the TCP session is not deliberately closed just because that request timed out.

### Scope of the setting

The Silent option suppresses **all Modbus error responses**: client-mode connection failures, malformed TCP/UDP replies, timeouts, received Modbus exceptions and server-mode invalid/missing RTU replies. Normal successful responses are still forwarded. The diagnostics log records such errors as `ERROR SILENT` along with `TCP ERROR`, `UDP ERROR`, `RTU ERROR` or `CRC ERROR` where applicable. Invalid RTU master requests are always discarded without forwarding.

## Multi-client scheduling

The four TCP sessions may remain connected at the same time. The RS-485 line is half-duplex and shared, so the gateway cannot execute four RTU transactions concurrently. It reads request data for each client slot, then processes one complete ready request at a time. The starting slot rotates after each completed transaction to reduce starvation when several clients are active.

This is **concurrent TCP connectivity, not parallel RTU execution**. Applications should use realistic polling rates and timeouts, especially when multiple masters poll the same RTU devices.

## Unit ID rules

| Flow | RTU address used | TCP Unit ID in response |
|---|---|---|
| RTU → TCP (Client mode) | Original RTU request address | Same value as RTU request address |
| TCP → RTU (Server, Automatic) | Incoming TCP Unit ID | Original incoming TCP Unit ID |
| TCP → RTU (Server, Manual) | Configured manual RTU ID (1–247) | Original incoming TCP Unit ID |

The RTU ID controls are hidden in TCP Client mode because that direction always preserves the RTU address from the incoming request.

## Function codes currently parsed by the gateway

The request/response length handling in the current source includes these standard function codes:

| Function code | Operation |
|---:|---|
| 01 | Read Coils |
| 02 | Read Discrete Inputs |
| 03 | Read Holding Registers |
| 04 | Read Input Registers |
| 05 | Write Single Coil |
| 06 | Write Single Register |
| 15 (0x0F) | Write Multiple Coils |
| 16 (0x10) | Write Multiple Registers |
| 23 (0x17) | Read/Write Multiple Registers |

Other function codes are not covered by the current RTU request-length parser and may not be handled correctly. Validate your device's required function codes before deployment.

## Frame diagnostics

- The rolling in-memory log retains the latest 64 events.
- Direction labels include RTU RX, RTU TX, TCP RX, TCP TX and MANUAL RTU.
- Times are `millis()` values (milliseconds since boot), not calendar timestamps.
- The browser refreshes the log periodically and performs an RTU CRC check for displayed RTU frames.
- The text export downloads the currently retained entries (up to 64); it is not a permanent or complete packet capture.

## RTU CRC validation

The gateway validates **CRC16**, slave Unit ID, function code, reply length and byte count before forwarding or caching any downstream Modbus RTU reply (TCP Server and UDP Server). CRC-invalid and malformed RTU frames are discarded, never forwarded and never cached. In Standard mode, the gateway can generate a *new* exception PDU `0x0B` in place of the invalid reply; in Silent mode it sends **nothing**. An incoming CRC-invalid RTU master request is discarded in Client mode before forwarding or looking up cache. Valid slave exception responses are never cached; they are forwarded only in Standard mode. Modbus TCP/UDP ADUs have an MBAP header and **no RTU CRC**, so their PDU is checked structurally.


## TCP Client asynchronous response handling

For normal RTU-originated requests in TCP Client mode, the gateway sends the TCP request and receives the MBAP header and response PDU incrementally from the ESPHome loop. It does not block the loop while waiting for the remote server response. The initial connection/reconnection and the physical RS-485 response transmission remain synchronous. See [Client TCP asynchronous handling](CLIENT_TCP_ASYNC.md).

## UDP Client — RTU to UDP

1. Receive an RTU request and validate its CRC.
2. Encode its Unit ID and PDU into one MBAP datagram and send it to the configured numeric IPv4 address and UDP port.
3. Accept a response only from the configured IP/port with matching transaction and Unit IDs.
4. Convert the valid reply to RTU and transmit it on RS-485.

UDP is connectionless: there is no persistent connection, and the gateway does not automatically retransmit UDP requests. Lost, late, malformed or unmatched replies must be handled through the configured timeout behavior.

## UDP Server — UDP to RTU

1. Bind the configured UDP port and validate the incoming MBAP datagram (protocol ID and length).
2. Resolve the RTU destination: automatic uses the incoming MBAP Unit ID; manual uses the configured RTU ID (1–247).
3. Send the request on RS-485 and wait for the RTU response, validating its CRC.
4. Reply to the source UDP IP/port, retaining the original incoming MBAP Unit ID.

The RS-485 bus is half-duplex and transactions are serialized. UDP Server's RTU wait remains synchronous, so a silent RTU slave may delay other gateway work until timeout. UDP does not guarantee delivery or ordering. The existing TCP Server four-connection limit is **not** a UDP session limit.

## Transport-aware web settings

**Persistent TCP connection** is available only in TCP Client. The Client and Server cache options are shared across TCP/UDP; the cache TTL input appears only while the currently selected mode's cache is enabled. For detailed limitations and user-reported tests, see [TCP/UDP development](TCP_UDP_DEVELOPMENT.md).
