# v1.4 — Reliable Modbus TCP, PSRAM diagnostics and incremental Web monitoring

**Project:** Modbus RTU ↔ IP Gateway for ESPHome / ESP32-S3  
**Source:** `main` (component version `1.4`, integrated 25 November 2026)  
**Previous published GitHub release:** `v1.3.8`

## Highlights

- **More reliable persistent TCP Client:** foreground Modbus RTU read requests (FC01–FC04) arriving during a background stale-while-revalidate refresh can be deferred without unconditionally disconnecting the persistent TCP session. At most one read is deferred, with a new TCP transaction ID and a bounded RTU response deadline. Writes retain priority and are **never automatically replayed**.
- **Improved recovery when Flow.io suspends Modbus TCP during OTA:** expired or superseded RTU reads are bounded and cannot produce late responses; background refresh continues safely when applicable. Existing automatic TCP reconnect and keepalive remain available.
- **PSRAM-backed diagnostics:** a fixed-size circular history of 256 frame events uses external PSRAM. Download the complete history from `/frames.txt`; inspect TCP/UART/Wi-Fi/heap and cache diagnostics through `/diag.txt` and `/diag.json`.
- **Lighter Web frame monitor:** `/frames?after=<seq>&epoch=<epoch>` returns incremental updates instead of re-sending the whole log. The UI shows the latest 64 entries, handles resets and rollovers, suspends polling for hidden tabs, and avoids overlapping frame requests.
- **Readable timestamps and settings:** frame capture and TXT export use local `YYYY-MM-DD HH:mm:ss.SSS` timestamps after asynchronous NTP synchronization, with configurable persistent timezone. Web configuration changes can be persisted safely and applied after active Modbus transactions as appropriate.
- **Cache and diagnostics refinements since v1.3.8:** fairer background refresh scheduling, shorter bounded TCP connect attempts for foreground reads, and additional status counters while retaining validation of Modbus frames and CRC.

## Compatibility

- ESPHome external component for ESP32-S3 with Arduino framework.
- Reference board: Waveshare ESP32-S3-RS485-CAN (default RS-485 GPIO TX17, RX18, enable21).
- TCP Client, TCP Server, UDP Client and UDP Server remain selectable; the most extensive recent changes concern **TCP Client with persistent connection and background read cache**.
- Existing ESPHome configuration and stored Web settings are intended to remain compatible.
- No prebuilt firmware binary is included: compile/install your own ESPHome firmware for your board and secrets.

## Install / upgrade (Home Assistant / ESPHome)

After the **GitHub tag `v1.4` is created**, pin the component in the device YAML:

```yaml
external_components:
  - source: github://stephdust/modbus-rtu-ip-gateway@v1.4
    components: [modbus_gateway]
```

Alternatively, `@main` retrieves the latest source, but does not pin a reproducible release. Recompile and install via ESPHome Device Builder (USB or OTA). Updating a GitHub repository does **not** update an already-flashed device.

Full setup: [ESPHome / Home Assistant installation guide](INSTALL_HOME_ASSISTANT.md).

## Validation and known limitations

- The underlying `1.3.16-rc2` source was exercised on real ESP32-S3 hardware on **25 November 2026**. Short field captures showed a persistent TCP session, no observed Wi-Fi/IP loss, CRC errors or UART overflows, and 79 incremental responses out of 86 `/frames` requests.
- The `1.4` source version bump introduced **no additional Modbus algorithm changes** beyond that tested source.
- **Not yet validated:** 12–24 hour endurance, complete TCP/UDP Client/Server regression, all real-world write cases, and a separate firmware build/device test specifically labeled `1.4`.
- The diagnostic Web server remains synchronous; large exports and heavy browser refresh may temporarily increase loop latency. Use frame monitoring primarily for diagnosis, not permanent supervision.
- The stale-while-revalidate cache can return older validated readings within its configured age limit. Disable it when live safety-critical measurements are required.

## Documentation

- [French 1.4 integration guide](MAIN_1.4_FR.md)
- [Full changelog](../CHANGELOG.md)
- [TCP read preemption and PSRAM](TCP_PREEMPT_PSRAM_FIX_FR.md)
- [Incremental Web frame monitor](WEB_INCREMENTAL_FRAMES_FR.md)
- [Field observations from 1.3.16-rc2](MAIN_1.3.16_RC2_FR.md)

---

## Résumé en français

La **v1.4** consolide les corrections de la connexion Modbus TCP persistante, la gestion des lectures RTU pendant les rafraîchissements du cache, le journal des trames en **PSRAM (256 entrées)** et l'affichage Web incrémental. Les journaux et exports conservent les horodatages à la milliseconde, avec fuseau horaire configurable. Les écritures Modbus ne sont jamais rejouées automatiquement.

La version repose sur le code précédemment testé en `1.3.16-rc2` ; les tests de stabilité longue durée et la validation exhaustive des modes TCP/UDP restent à effectuer. Après publication du tag GitHub `v1.4`, utiliser `@v1.4` dans le YAML ESPHome puis recompiler et installer le firmware.
