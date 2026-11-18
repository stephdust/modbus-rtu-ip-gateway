# Modbus RTU/IP Gateway — TCP and UDP implementation

Version **1.2.6** on **`main`** includes TCP and UDP Client/Server transport modes. The earlier TCP-only v1.1.15 is part of the repository history; this document originated on the feature branch and now describes the merged implementation.

## Configuration

In the custom component YAML, use `transport: TCP` (default) or `transport: UDP`, together with `tcp_mode: CLIENT` or `tcp_mode: SERVER`. The existing `tcp_host`, `tcp_port` and `tcp_timeout` options are also used for UDP. The web UI provides a TCP/UDP selector; settings are stored in NVS. Restart or reapply the configuration to rebind the transport.

- **UDP Client**: incoming Modbus RTU requests are encoded as MBAP datagrams and forwarded to the configured IPv4 server. Replies are accepted only from the configured IP/port with matching transaction and Unit IDs. No automatic retries are performed.
- **UDP Server**: binds the configured port, validates one complete MBAP ADU per datagram, forwards it to the RTU bus, and sends the reply to the source IP/port. Automatic RTU ID mode uses the MBAP Unit ID; manual mode overrides it. The MBAP Unit ID in the response remains the original value.
- **Cache**: the existing client/server read caches are shared across the selected transport, and writes invalidate matching entries. Version 1.2.6 includes a Fixed/Adaptive TTL selector (adaptive = 80% of observed per-request polling interval, bounded to 50–10,000 ms); this change has not yet been compiled or tested on hardware.
- **TCP**: remains the default transport.

## Validation status (15 November 2026)

The maintainer reports a successful ESPHome firmware build, installation on the ESP32-S3, and functional tests of **TCP Client, TCP Server, UDP Client and UDP Server**. Reported UDP checks include FC03/FC04 reads, FC06/FC16 writes, automatic/manual RTU ID and caching. These are user-reported functional tests, not independent protocol conformance or robustness certification. Timeout, malformed-frame, network-loss and concurrent-client stress testing remain outstanding. Do not deploy on a production Modbus network without validating the target installation.

The UDP Server currently uses the existing synchronous RTU response wait (up to `tcp_timeout`), which can delay web UI responsiveness while a slave is silent. UDP is connectionless and does not guarantee delivery or ordering. Only numeric IPv4 addresses are supported for UDP Client configuration. UDP packets with invalid MBAP lengths or protocol ID are discarded.

## Web interface behavior

Section **1. Modbus IP** provides the TCP/UDP transport selector and Client/Server mode selector. The **Persistent TCP connection** checkbox is **visible and enabled only in TCP Client mode**; it is hidden and disabled for TCP Server, UDP Client and UDP Server. The UI updates immediately when either selector changes, without reloading the page. The saved TCP preference is retained when switching to UDP and is not overwritten by a hidden disabled checkbox.

The header uses the RTU ↔ IP gateway artwork with caches on both sides. The four section thumbnails (Modbus IP, Modbus RTU, ESP32 Configuration and frame diagnostics) use a consistent blue/cyan style and matching dimensions. The Modbus IP thumbnail now explicitly illustrates **TCP / UDP**, replacing the legacy TCP-only thumbnail. FR/EN and light/dark UI controls remain available.

## Suggested validation

1. Build with ESPHome 2026.10.1 / Arduino on the Waveshare ESP32-S3-RS485-CAN target.
2. Run `g++ -std=c++11 -Wall -Wextra -pedantic -Icomponents/modbus_gateway tests/test_mbap_codec.cpp -o /tmp/test_mbap && /tmp/test_mbap` on a host machine.
3. Test UDP Client against a known Modbus UDP server: reads, writes, timeouts, wrong source IP/port, transaction ID and Unit ID.
4. Test UDP Server against a real RTU slave: automatic/manual RTU ID, concurrent UDP masters, read cache, write invalidation, CRC errors and silent timeout.
5. Regression-test TCP Client and TCP Server, persistent connections, diagnostics, NVS persistence and the FR/EN interface.

## État du développement (français)

La branche a été compilée et installée sur ESP32-S3. Selon les essais communiqués par l'utilisateur, les **quatre modes TCP/UDP Client/Serveur fonctionnent**, y compris les lectures FC03/04, écritures FC06/16, ID RTU automatique/manuel et cache en UDP. Les tests de robustesse restent à effectuer. La version TCP/UDP v1.2.1 a été publiée. Le code v1.2.6 sur `main` ajoute le TTL adaptatif, qui reste à compiler et valider.

Dans l'interface, **Connexion TCP persistante** est affichée uniquement en **TCP Client** et masquée en TCP Serveur, UDP Client et UDP Serveur ; le changement est immédiat. Le réglage TCP enregistré est conservé lors du passage en UDP. Les quatre petits logos sont harmonisés avec le logo principal, et le pictogramme Modbus IP indique désormais **TCP/UDP**.


> **Web UI v1.2.7:** The Mode selector is transport-neutral: **Client (RTU → IP)** or **Server (IP → RTU)**. Choose **TCP** or **UDP** independently in Transport. Earlier v1.2.6 screenshots remain historical references and may display older labels.
