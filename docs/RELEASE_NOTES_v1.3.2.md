# v1.3.2 — Modbus RTU ↔ IP Gateway

18 November 2026 · Source release for ESPHome / ESP32-S3 (Arduino framework)

## Highlights

- **Optional bounded stale-while-revalidate cache** in TCP Client mode (RTU → TCP). The gateway can return a previously validated Modbus read PDU to the RTU display immediately, even when the conventional cache TTL has expired, provided it remains within the configured maximum data age.
- **Background Modbus TCP refresh** updates a stale cache entry for later requests. The refresh result **never generates a second, unsolicited RTU reply**. When a foreground cache miss must take priority, the background TCP request is cancelled and its connection closed.
- **RTU reply deadline** suppresses responses that cannot be transmitted completely within the configured time window. It accounts for configured baud, data bits, parity, stop bits and transmission time.
- **More diagnostics:** `/cache-stats` exposes `fresh_hits`, `stale_hits`, `refresh_pending`, `refresh_started`, `refresh_success`, `refresh_failure`, `late_rtu_dropped` and age/deadline settings. The frame log identifies cache refreshes and dropped late responses.
- **Web UI FR/EN:** optional background refresh, maximum data age and RTU response window can be configured in the gateway's own port-80 interface.
- **Existing protections kept:** only structurally correct reads (FC01–FC04) populate the cache; failed responses and Modbus exceptions are never cached; RTU CRC checks, write invalidation, Standard/Silent error policy, and existing TCP/UDP modes remain.

## Field test / validation

**Reported working in the maintainer's installation on 18 November 2026:** the new background-refresh cache operated successfully with a physical Modbus RTU display, an ESP32-S3 RS485 gateway and the Flow.io Modbus TCP server. Earlier captures had shown missed RTU timing windows with conventional TTL cache; the maintainer reports that the new cache is working well.

This is **user-reported field validation**, not an independent protocol-conformance result. No complete long-duration soak test, network-loss stress test, safety certification or full regression matrix for all TCP/UDP Client/Server combinations is claimed.

## Configure

1. Compile the sample `my-modbus-gateway.yaml` with ESPHome (Arduino/ESP32-S3) using the `modbus_gateway` external component from this release or from `main`.
2. Open `http://<gateway-ip>/` (Modbus Web UI, port 80).
3. Select **TCP Client** and enable both **Persistent TCP connection** and **Client cache**.
4. Enable **Client cache: respond immediately and refresh in background**.
5. Set the normal cache TTL, **maximum data age** (1,000–30,000 ms; default 10,000 ms), and **RTU reply window** (250–2,000 ms; default 500 ms). Keep values appropriate for the display and application.
6. View `http://<gateway-ip>/cache-stats` and the frame capture to verify `CACHE STALE`, `CACHE REFRESH OK` and the deadline counters.

The new mode is **disabled by default**. Existing TCP Server, UDP Client and UDP Server cache behavior is unchanged. The stock offline ESPHome v3 diagnostic UI is on port 8080; it does not use custom JavaScript injection. The `Interface web` text sensor in the YAML exposes the port-80 URL.

## Important precautions

Cached data may be older than the device's current state, up to the configured maximum data age. **Do not use stale replies for safety-critical or always-live control measurements.** Invalid-CRC or structurally invalid responses cannot become new cached values, but plausible yet inaccurate device-supplied values cannot be detected by structural validation.

This release provides the ESPHome source component and sample YAML. **No independently validated precompiled firmware binary is claimed or attached.**

For more information: [README](../README.md), [Cache behavior](CACHE.md), [Quick start](QUICK_START.md), [Changelog](../CHANGELOG.md).

---

**Résumé FR :** Nouveau cache TCP Client avec réponse RTU immédiate et rafraîchissement TCP en arrière-plan, protection contre les réponses RTU tardives, diagnostics étendus et réglages Web FR/EN. Fonctionnement confirmé par le mainteneur sur son afficheur Modbus RTU avec Flow.io. Les essais de robustesse et de sécurité restent à effectuer.
