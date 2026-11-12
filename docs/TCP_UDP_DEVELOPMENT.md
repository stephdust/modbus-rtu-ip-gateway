# Modbus RTU/IP Gateway — TCP and UDP development branch

This branch extends the existing ESPHome/Arduino ESP32-S3 gateway with Modbus-over-UDP transport. The production TCP firmware remains on `main`.

## Configuration

In the custom component YAML, use `transport: TCP` (default) or `transport: UDP`, together with `tcp_mode: CLIENT` or `tcp_mode: SERVER`. The existing `tcp_host`, `tcp_port` and `tcp_timeout` options are also used for UDP. The web UI provides a TCP/UDP selector; settings are stored in NVS. Restart or reapply the configuration to rebind the transport.

- **UDP Client**: incoming Modbus RTU requests are encoded as MBAP datagrams and forwarded to the configured IPv4 server. Replies are accepted only from the configured IP/port with matching transaction and Unit IDs. No automatic retries are performed.
- **UDP Server**: binds the configured port, validates one complete MBAP ADU per datagram, forwards it to the RTU bus, and sends the reply to the source IP/port. Automatic RTU ID mode uses the MBAP Unit ID; manual mode overrides it. The MBAP Unit ID in the response remains the original value.
- **Cache**: the existing client/server read caches are shared across the selected transport, and writes invalidate matching entries.
- **TCP**: remains the default transport.

## Important limitations

**Experimental / not yet validated on ESP32-S3 hardware.** The source has been committed, but an ESPHome/Arduino firmware build and physical RS485 tests have **not** been completed. Do not deploy on a production Modbus network before verification.

The UDP Server currently uses the existing synchronous RTU response wait (up to `tcp_timeout`), which can delay web UI responsiveness while a slave is silent. UDP is connectionless and does not guarantee delivery or ordering. Only numeric IPv4 addresses are supported for UDP Client configuration. UDP packets with invalid MBAP lengths or protocol ID are discarded.

## Suggested validation

1. Build with ESPHome 2026.10.1 / Arduino on the Waveshare ESP32-S3-RS485-CAN target.
2. Run `g++ -std=c++11 -Wall -Wextra -pedantic -Icomponents/modbus_gateway tests/test_mbap_codec.cpp -o /tmp/test_mbap && /tmp/test_mbap` on a host machine.
3. Test UDP Client against a known Modbus UDP server: reads, writes, timeouts, wrong source IP/port, transaction ID and Unit ID.
4. Test UDP Server against a real RTU slave: automatic/manual RTU ID, concurrent UDP masters, read cache, write invalidation, CRC errors and silent timeout.
5. Regression-test TCP Client and TCP Server, persistent connections, diagnostics, NVS persistence and the FR/EN interface.

## État du développement (français)

Les modes UDP Client et UDP Serveur ont été ajoutés au code sur cette branche. **Ils ne sont pas encore compilés ni validés sur matériel.** La version TCP fonctionnelle reste sur `main`. Avant une mise en service, compiler avec ESPHome/Arduino et effectuer les essais RTU/UDP et les tests de non-régression TCP.
