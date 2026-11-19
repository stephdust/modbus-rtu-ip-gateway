# Changelog

## 1.3.5 — TCP destination connection status colors (2026-11-19; source on `main`)

- Show TCP Client destination status in red when disconnected and green when connected, including the status indicator and destination text.
- Keep TCP Server and UDP modes' existing display behavior; this is a Web UI-only change.
- Compilation and device verification pending.

## 1.3.4 — Bidirectional frame language display (2026-11-19; source on `main`)

- Fix French frame monitoring after switching back from English: translate firmware-origin English diagnostics explicitly into French.
- Preserve English log text when EN is selected and refresh the panel immediately on language changes.
- Keep Modbus protocol, cache, and timing behavior unchanged. Device validation pending.

## 1.3.3 — Frame monitoring language fix (2026-11-19; source on `main`)

- Fix the RTU/IP frame monitoring panel showing French event and timing messages when English is selected.
- Preserve firmware-origin English log text in EN mode; retain the existing French translations in FR mode.
- Refresh frame monitoring immediately after switching languages.
- No changes to Modbus transactions, timing, caching, or serial/network behavior. Compilation and device verification pending.

> **Documentation uniquement (19 novembre 2026, branche `main`) :** ajout du [guide ESPHome Device Builder sous Home Assistant](docs/INSTALL_HOME_ASSISTANT.md) — copier le seul YAML, configurer les secrets, récupérer automatiquement le composant depuis GitHub, compiler/flasher USB ou OTA. **Pas de changement du firmware ni du numéro de version v1.3.2.**

## 1.3.2 — Optional bounded background refresh and documentation cleanup (2026-11-18; [GitHub release published](https://github.com/stephdust/my-modbus-rtu-ip-gateway/releases/tag/v1.3.2))

- Added opt-in TCP Client stale-while-revalidate cache: validated read results can be served up to a configured maximum age while TCP refreshes run in the background without unsolicited RTU replies.
- Added a configurable RTU response deadline to suppress late replies; cache/refresh diagnostic counters and FR/EN Web settings are included.
- Default gateway behavior remains unchanged until the user enables the new cache mode. **The maintainer confirmed that v1.3.2's new cache works well on the physical RTU display via the Flow.io Modbus TCP server (18 November 2026).** This is one successful field setup; extended soak, network failure, security/safety and full TCP/UDP regression tests have not been independently demonstrated.
- Updated README and guides: **ESPHome port 8080 uses the stock local Web UI, with no injected JavaScript**; the `Interface web` text sensor exposes the port-80 URL derived from Wi-Fi IP.
- Corrected outdated version/validation statements and the documented scope of the Modbus **Silent** error policy; preserved historical v1.2.7 screenshots.


## 1.3.1 — CRC-first forwarding, all-error silence and cache correctness (2026-11-18; source on `main`)

- Silent now suppresses **every Modbus error response** in Client and Server modes, TCP and UDP; successful replies are unaffected.
- CRC-invalid RTU frames and replies with wrong Unit ID or malformed PDU are never forwarded or cached. Standard mode may generate a new gateway exception (not relay corrupt bytes).
- Added a host-testable policy and C++11 tests covering corrupt CRC, exception handling, reply lengths, cache TTL expiry and duplicate cache keys.
- Fixed cache lookup/storage to favor the freshest unexpired key and avoid extending expired data with adaptive TTL updates. Strict response validation gates cache insertion.
- Updated FR/EN Web labels; ESPHome firmware/hardware validation remains pending.

## 1.3.0 — Stable baseline (2026-11-18; source on `main`)

- Consolidate the Modbus RTU ↔ TCP/UDP gateway and bilingual web UI.
- Keep ESPHome's stock offline v3 web interface on port 8080, with no injected JavaScript.
- Restore the Modbus interface URL text sensor by publishing from the ESPHome Wi-Fi IP sensor when its value changes.
- Preserve the gateway's own configuration and diagnostics UI on port 80.
- Version promoted following user confirmation that the IP display now works. No new hardware or compilation validation performed for this version bump.

## 1.2.9 — English Web UI translation fixes (2026-11-18; source on `main`, not yet released)

- Complete English labels and descriptions for TTL settings, status/connections, counters, diagnostics and frame panels.
- Translate the Refresh action and fix the TCP/UDP diagnostics caption.
- Use explicit `data-fr`/`data-en` text for elements that already declare both languages.
- No Modbus protocol changes. Compilation and hardware verification pending.

## 1.2.8 — Safe recovery of stale persistent TCP connections (2026-11-18; source on `main`, not yet released)

- In TCP Client mode with persistent connections enabled, if a reused socket fails during request write, disconnects before the full reply, or times out waiting for the reply, reconnect and retry the original Modbus read **once** for function codes **03** (holding registers) and **04** (input registers).
- Never automatically replay Modbus write requests; they may have been executed even when the TCP reply was lost.
- Emit `TCP RETRY` in the frame/event log; after one failed retry, retain existing Modbus exception / silent-timeout behavior.
- UDP and TCP Server behavior unchanged. Compilation and device testing pending.

## 1.2.7 — Transport-neutral mode selector (2026-11-18; source on `main`, released as v1.2.7)

- Clarified the Web UI: Mode now shows **Client (RTU → IP)** or **Server (IP → RTU)**, while Transport independently selects **TCP** or **UDP**.
- Corrected English translations and cache descriptions to avoid calling UDP traffic TCP.
- No protocol or transport logic changed. Compilation and hardware validation pending.

## 1.2.6 — Manual frame response layout (2026-11-18; source on `main`, release publication pending)

- Keep the manual Modbus IP response label and response output inside the same panel as the manual frame input and Send button.
- Rebuilt four v1.2.6 simulated Web UI illustrations (TCP/UDP, Client/Server), reflecting unnumbered headings, adaptive TTL, and the separated manual send / frame monitoring panels; these are illustrations, not browser screenshots.
- Fix premature panel closing markup. No Modbus protocol behavior changes. Build and hardware validation pending.

## 1.2.5 — Clearer Web UI sections (2026-11-18; source on `main`, release publication pending)

- Removed numeric prefixes from the four main Web section headings, in French and English.
- Visually separated manual Modbus IP frame transmission from RTU/IP frame monitoring and capture, with dedicated titled panels.
- Preserved the existing manual-send, listening, clear-log and TXT-export functions. Build and hardware verification pending.

## 1.2.4 — Frame diagnostics action bar (2026-11-18; source on `main`, release publication pending)

- Moved Stop/Resume RTU listening next to Clear frame display and Export frame capture (.txt), retaining the live listening status on the same responsive action row.
- No change to Modbus transport or cache behavior. Compilation and hardware validation pending.

## 1.2.3 — Live cache diagnostics (2026-11-18; source on `main`, release publication pending)

- Added live active-cache occupancy (`0 / 64` when disabled) and effective TTL range to the Counters / Diagnostics Web panel, with a lightweight JSON status endpoint. Counts exclude expired entries and entries belonging to the inactive Client/Server direction. Firmware build and hardware validation pending.

## 1.2.2 — Fixed and adaptive cache TTL (2026-11-18; source on `main`, release publication pending)

- Added a Fixed/Adaptive response-cache TTL selector in the Web UI for Client and Server modes over TCP/UDP.
- Fixed TTL retains the 50–10,000 ms range and 500 ms default. Adaptive TTL estimates 80% of the observed per-key polling interval, bounded to the same range, and falls back to the fixed value until a usable interval is available.
- Persist the selected TTL mode in NVS; adaptive observations and cache entries remain in RAM.
- Updated README, quick start, cache behavior and TCP/UDP documentation, and added four explicitly simulated v1.2.2 Web UI illustrations. **Compilation and hardware validation are pending.**

## 1.2.1 — TCP/UDP gateway (2026-11-15)

- Published on `main` under the renamed repository `stephdust/my-modbus-rtu-ip-gateway`.
- Redesigned midnight-blue Web UI with status/connection table, colored RTU/TCP/UDP frame diagnostics, and aligned top toolbar with save, restart, refresh, theme and language controls.
- Added documentation simulations for TCP Client, TCP Server, UDP Client and UDP Server.


- Harmonized the main RTU ↔ IP header artwork and all four section thumbnails; replaced the TCP-only network thumbnail with a TCP/UDP illustration.
- Display the persistent TCP connection option only in TCP Client mode; hide and disable it for TCP Server and both UDP modes, preserving the stored TCP preference.
- Updated TCP/UDP development documentation and recorded maintainer-reported functional tests of all four transport/mode combinations.
- No changes to Modbus protocol processing in this patch; robustness testing remains pending.

## 1.2.0 — TCP/UDP development branch

- Added selectable TCP/UDP transport with Client and Server modes on the feature branch.
- Extended the existing Modbus gateway web interface and cache handling to the selected transport.

## 1.1.15

- Reworked the Modbus TCP settings into two explicit vertical columns so conditional fields no longer leave one column sparse or push all remaining options to one side.
- Balanced the two columns by grouping mode, server address, port and timeout behavior on the left; timeout, persistent connection and cache settings on the right.
- Switched to a single stacked column on narrow/mobile screens.
- No Modbus communication behavior or configuration semantics changed.

## 1.1.14

- Initial two-column Web UI layout adjustment.

## 1.1.13

- Added a bilingual Web UI option for timeout behavior, shared by TCP Client and TCP Server modes.
- Default `Standard` behavior sends a Modbus gateway exception when the remote device does not respond before the configured timeout.
- Optional `Silent` behavior sends no RTU response in Client mode or no TCP response in Server mode when the remote device times out; the upstream master/client then applies its own timeout.
- The setting is persisted in ESP32 preferences and survives restart. The silent option applies only to a missing/incomplete remote response timeout; invalid RTU CRC frames and other transport/protocol errors are still handled and reported.
- Client-mode remote TCP response timeouts use gateway exception `0x0B` in Standard mode; other client-side transport/protocol failures retain `0x0A`. In Server mode, an RTU slave timeout uses TCP gateway exception `0x0B` in Standard mode.
- Updated the GitHub documentation for the timeout policy, diagnostics, cache invalidation rules and v1.1.13 defaults.

## 1.1.12

- Replaced broad Unit ID cache invalidation with write-range-aware invalidation for successful standard Modbus write operations.
- Function `05` and `0F` writes invalidate only overlapping cached Read Coils (`01`) entries.
- Functions `06`, `10`, `16`, and `17` invalidate only overlapping cached Read Holding Registers (`03`) entries.
- Non-overlapping cached ranges and read-only spaces (Discrete Inputs `02` and Input Registers `04`) are retained.
- Unknown or malformed non-read operations conservatively invalidate all cached reads for the affected Unit ID in the active communication direction.
- Failed write operations and Modbus exception responses do not invalidate cache entries.
- Cache invalidation diagnostics now include the write function, address range, affected read function and number of removed entries.

## 1.1.11

- Made the Web UI response-cache controls mode-aware: TCP Client mode shows only the Client cache switch; TCP Server mode shows only the Server cache switch.
- Switching modes updates the visible cache option immediately without a page reload.
- Saving settings updates only the active mode's cache preference and preserves the hidden mode's previously saved preference.
- Clarified in the bilingual Web UI and documentation that the cache TTL is shared and the inactive mode's setting is retained.
- Updated the GitHub documentation for persistent TCP sessions, cooperative asynchronous Client TCP response handling, independent response caches, RTU CRC rejection, timing diagnostics, and the current Waveshare reference board.

## 1.1.10

- Explicitly reject RTU frames with invalid CRC in both TCP Client and TCP Server modes.
- Client mode logs `CRC ERROR` and drops a corrupt RTU master request before forwarding it to TCP or consulting/updating the response cache.
- Server mode logs the invalid RTU response (with CRC visibly marked in the Web trace), drops the corrupt response before TCP forwarding/cache storage, and returns Modbus TCP gateway exception `0x0B` to the requesting client.
- CRC diagnostics include received and calculated CRC values, frame length, Unit ID/function where available, and a cumulative invalid-frame count since boot.
- Incomplete/timeout RTU responses are logged separately as `RTU ERROR` and are not counted as CRC failures.

## 1.1.9

- Refactored the normal TCP Client (RTU → TCP) transaction path into a cooperative, non-blocking response state machine.
- TCP MBAP headers and response PDUs are now collected incrementally from `loop()` without waiting in a blocking read loop.
- The Web UI, Wi-Fi servicing, and other ESPHome loop work continue while the remote Modbus TCP server is preparing a response.
- The configured TCP timeout is enforced separately for the MBAP header and response PDU stages, preserving the previous timeout model.
- Preserved persistent TCP reuse, response-cache behavior, Modbus TID/Unit ID/MBAP validation, and gateway-exception replies on failed transactions.
- RTU requests received while a TCP transaction is pending are buffered (up to 2 KiB) and handled sequentially after the previous response is returned.
- Manual Web UI test requests remain synchronous and are rejected with HTTP 409 while an RTU-originated Client TCP transaction is active, preventing concurrent use of the same socket.
- TCP Server mode and its four-client scheduler are unchanged.
- Initial/reconnect `WiFiClient::connect()` and RS-485 transmission completion remain synchronous; the long wait for the remote TCP response is now cooperative/non-blocking.

## 1.1.8

- Added an optional 64-entry response cache for TCP Client mode (RTU → TCP) and TCP Server mode (TCP → RTU), with independent enable switches.
- Added a shared configurable cache TTL (50–10,000 ms; default 500 ms). Both caches are disabled by default.
- Cache keys include direction, effective RTU Unit ID, read function, start address and quantity.
- Only successful, structurally valid Modbus read responses (functions 01–04) are stored; exceptions and write/read-write functions are never cached.
- Successful non-read operations invalidate cached responses for the affected Unit ID.
- TCP Server cache hits retain the requesting client's Transaction ID and TCP Unit ID while bypassing the RTU bus.
- Cache hit/miss/store/invalidation events are visible in the frame diagnostics and text export.
- Cache contents are volatile and are cleared after configuration is saved or the ESP32 restarts.

## 1.1.7

- Fixed a malformed JavaScript translation-map entry introduced in 1.1.6; the page script now parses correctly so frame refresh, language switching and Web UI actions run again.
- Kept the 1.1.6 byte-timing instrumentation and transaction behavior unchanged.

## 1.1.6

- Added first-byte latency and first-to-last receive-span diagnostics for TCP Client responses.
- Added RTU request first-to-last receive-span diagnostics in TCP Client mode.
- Added RTU response first-byte latency and first-to-last receive-span diagnostics in TCP Server mode.
- Added TCP request first-to-last receive-span diagnostics in TCP Server mode.
- Receive timestamps are collected alongside the existing buffers without changing transaction timeouts or RS-485 guard intervals.
- Clarified timing output in the Web trace and text export; measurements represent MCU-side receive observations, not a physical-layer oscilloscope measurement.

## 1.1.5

- Added per-transaction latency measurements in both TCP Client and TCP Server modes.
- Client diagnostics report TCP connect/reuse time, request write time, TCP response wait and total TCP transaction time.
- Server diagnostics report queue wait, RS-485 transmit time, RTU response wait, TCP response write and total processing time.
- Added end-to-end RTU Client transaction timing.
- Increased the rolling diagnostics history from 32 to 64 entries so timing events do not immediately displace frame records.
- Reserved reusable RTU/TCP buffers during setup to reduce heap allocations during transactions.
- Kept TCP_NODELAY enabled and preserved Modbus RTU direction-control guard times and the existing timeout behavior.
- The 1 ms cooperative polling delays are intentionally retained for this measurement release; timing data should guide any further tuning without risking Wi-Fi/task scheduling or RTU reliability.

## 1.1.4
- Add optional persistent TCP client mode (enabled by default) to avoid a new TCP handshake for each RTU request.
- Reuse an active TCP socket and automatically establish a new connection after a disconnect.
- Display the actual Wi-Fi IP address from ESPHome's WiFiComponent in TCP diagnostics.
- Add a Web UI reboot button using the existing reboot countdown page.

## 1.1.3

- Client TCP: no longer blocks connection attempts based only on `WiFi.status()`.
- Added Wi-Fi status and local IP details to the `TCP CONNECT` frame diagnostic.
- The actual TCP socket connection result now determines whether the remote Modbus TCP server is reachable.

## 1.1.2

- Added Client TCP connection diagnostics to the frame trace and text export.
