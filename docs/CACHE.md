# Modbus response cache

The gateway includes an optional response cache for each operating mode. The cache is intended to reduce repeated polling latency when a master asks for the same read data more than once within a short interval.

## Independent mode switches

The Web UI exposes two independent settings in **1. Modbus IP**. To keep the page unambiguous, it displays only the setting that belongs to the currently selected Client/Server mode:

- **IP Client selected:** the UI shows **Enable cache in Client IP mode (RTU → IP)**, which reuses a previously received Modbus TCP read response for a matching RTU request.
- **IP Server selected:** the UI shows **Enable cache in Server IP mode (IP → RTU)**, which reuses a previously received RTU read response for a matching TCP request.

Both settings are independent and disabled by default. Changing the selected Client/Server mode updates the visible switch immediately without reloading the page. Saving in one mode updates only that mode’s cache setting; the hidden mode’s setting is preserved in NVS and appears again when that mode is selected.

## Cache controls and historical v1.2.7 Web UI behavior

In **TCP Client** or **UDP Client**, only the **Client cache** switch is shown; in **TCP Server** or **UDP Server**, only the **Server cache** switch is shown. The inactive direction's saved setting is preserved. The **TTL mode** selector appears only when the selected direction's cache is enabled; the numeric TTL input appears only for Fixed TTL.

For actual v1.2.7 screenshots of all four modes, see the [Web interface gallery](QUICK_START.md#captures-réelles-de-linterface-web-v127--quatre-modes).

## TTL modes: fixed or adaptive (v1.2.7)

The Web interface offers a **TTL mode** selector whenever the active Client/Server cache is enabled:

- **Fixed TTL** (default, backward compatible): manually enter a TTL from 50 to 10,000 ms (default 500 ms).
- **Adaptive TTL**: estimate the polling interval separately for each cache key (direction, Unit ID, function, start address and quantity), then choose a TTL of 80% of the observed interval, bounded to 50–10,000 ms. Until an interval has been observed, the configured fixed TTL is used as a fallback.

Fixed/adaptive TTL cache behavior has been observed in user-provided gateway logs; this does not constitute full hardware or functional certification. The separate bounded background-refresh option introduced in source v1.3.2 has not yet been ESPHome-compiled or verified with the physical display. The selected TTL mode is saved in ESP32 NVS. Adaptive estimates are held only in RAM and reset when the cache is cleared. The age of a cached response is always measured from the time it was fetched, never from a cache hit. Writes continue to invalidate affected cached reads.

**Important:** Adaptive TTL reduces unnecessary refreshes for repeated reads, but cannot guarantee fresh values if device data changes between polls. For safety-critical or rapidly changing registers, disable the cache or use a short fixed TTL. Adaptive behavior should be validated with the actual polling workload and device before production use.

## Time-to-live and capacity

A shared TTL mode and fixed fallback value apply to both directions. It can be set from **50 to 10,000 ms**; the default is **500 ms**. The cache holds up to **64 entries**. A cache hit does not refresh the stored timestamp, so frequently repeated reads cannot keep old data alive indefinitely. Entries expire relative to the time the response was actually fetched from the remote device.

The cache is stored in RAM only. It is cleared when the ESP32 restarts or when configuration is saved. Cache contents are not written to flash/NVS.

## Cache key

A cache entry is matched using:

- Communication direction (Client or Server mode)
- Effective RTU Unit ID
- Modbus function code
- Starting register/coil address
- Requested quantity

In Server mode, when manual RTU ID is selected, the effective configured RTU Unit ID is used as the device key. The TCP response still carries the Unit ID from the current TCP request, and the current client's Transaction ID is always used.

## What is cached

Only valid, structurally correct read responses for standard function codes are stored:

- `01` – Read Coils
- `02` – Read Discrete Inputs
- `03` – Read Holding Registers
- `04` – Read Input Registers

The response function code and byte count are checked before a response is stored. Modbus exception responses and malformed responses are not cached.

Write operations and read/write operations are never cached. After a successful standard write, invalidation is limited to cached entries whose address range overlaps the write range and whose read function addresses the corresponding writable data space:

- `05` (Write Single Coil) and `0F` (Write Multiple Coils) invalidate overlapping `01` (Read Coils) entries.
- `06` (Write Single Register), `10` (Write Multiple Registers), `16` (Mask Write Register), and `17` (Read/Write Multiple Registers) invalidate overlapping `03` (Read Holding Registers) entries. For `17`, the write start and quantity—not the read range—are used for invalidation.

Non-overlapping ranges and read-only spaces (`02` Discrete Inputs and `04` Input Registers) are retained. An unknown or malformed non-read operation falls back to invalidating all cached reads for that Unit ID in the active communication direction. Modbus exception responses and failed writes do not invalidate entries.

## Diagnostics

The **Counters / Diagnostics** dashboard now includes **Cache (entries)** and **Effective TTL**. When the cache for the selected Client/Server mode is disabled, the count is `0 / 64`; when enabled, it counts only unexpired entries for that direction. The TTL field shows the current minimum–maximum TTL among valid entries (or a single value when equal). An empty cache shows `—` for TTL. Both values refresh with the Web frame diagnostics, allowing the user to observe adaptive TTL changes as the cache fills. This is cache occupancy (entries), not RAM usage in bytes; it is not a guarantee that device values changed or that adaptive behavior is optimal.


The frame diagnostics and `.txt` export show cache events:

- `CACHE MISS` – no valid matching entry was found; the request is sent to the remote device.
- `CACHE STORE` – a valid read response was stored.
- `CACHE HIT` – a matching, unexpired response was reused.
- `CACHE INVALIDATE` – a successful write removed overlapping entries only; the log identifies the write function, address, quantity, affected read function and number of entries removed. Unknown/malformed write ranges are logged as a conservative full-Unit-ID invalidation.

On a cache hit, Client mode sends the cached PDU back over RTU. Server mode creates a fresh TCP MBAP header using the current request's Transaction ID and TCP Unit ID, and does not transmit anything on RS-485 for that request.

## Bounded stale-while-revalidate (v1.3.2)

In **TCP Client (RTU → TCP)** mode, enable **Client cache**, keep
**Persistent TCP connection** enabled, then select
**Client cache: respond immediately and refresh in background**.
This new feature is **disabled by default**. TCP Server, UDP Client and
UDP Server retain their previous cache behavior.

1. **Fresh hit:** reply immediately with the last validated read PDU.
2. **Stale hit:** if the entry is older than the fixed/adaptive TTL but below
   the maximum data age, reply immediately with the last validated PDU,
   and trigger a single background TCP refresh. On successful validation,
   the cache is updated **without transmitting another RTU frame**.
3. **Cache miss or age exceeded:** do not use old data. The gateway performs
   a foreground TCP transaction. A late RTU reply is dropped if its complete
   serial transmission cannot fit within the configured RTU reply window.
4. While a background refresh is pending, cached RTU requests continue to
   receive immediate answers. An uncached foreground request closes the
   background TCP socket before starting its own transaction.
5. Refresh starts are rate-limited to one per second. A failed refresh
   never generates an RTU error/exception frame.

**Configurable bounds:** maximum data age 1,000–30,000 ms (default 10,000),
and RTU reply window 250–2,000 ms (default 500). The maximum age is measured
from the **most recent validated TCP response**, never from a cache hit.
An RTU reply deadline includes the whole on-wire response at the configured
baud/parity/data/stop bits plus a guard. The timer is anchored to the
last captured RTU request byte, not to deferred parsing time.

The response cache is still restricted to structurally valid Modbus read
responses (FC01–FC04), including the function and byte count checks.
Exceptions, invalid PDU shapes, RTU CRC errors and failed transactions are
never cached. Writes invalidate relevant entries. **Do not enable** stale
serving for measurements that require always-current process/safety values.

**Diagnostics:** `GET /cache-stats` now also reports `swr_enabled`,
`max_age_ms`, `rtu_window_ms`, `stale_entries`,
`fresh_hits`, `stale_hits`, `refresh_pending`, `refresh_started`,
`refresh_success`, `refresh_failure`, and `late_rtu_dropped`.
Logs include `CACHE STALE`, `CACHE REFRESH`, `CACHE REFRESH OK`,
`CACHE REFRESH ERROR` and `RTU DEADLINE`.

This implementation is source- and host-test-reviewed but still requires an
ESPHome compilation and validation with the real display and RS485 bus.

## Data freshness warning

A cache hit intentionally returns a previous valid device response. The data can be as old as the applicable fixed or adaptive TTL. Choose a TTL appropriate for the process and its update rate. If stale data is unsafe or unacceptable, leave the corresponding mode's cache disabled. The cache is a performance option, not a substitute for device-side update timing or application-level freshness checks.

## Interaction with asynchronous Client TCP processing

From component release 1.1.9, normal RTU-originated Client TCP transactions receive the remote TCP response cooperatively through the ESPHome loop. Cache hits remain immediate and do not open or use the TCP socket. A manual transaction initiated from the Web UI still uses the synchronous diagnostic helper; it is refused with HTTP 409 while a normal Client TCP transaction is already in progress so that both operations cannot read from the same TCP stream concurrently.

## TCP and UDP (v1.2.7)

Both cache switches are **direction-specific**, not transport-specific: **Client** applies to TCP Client and UDP Client, and **Server** applies to TCP Server and UDP Server. The currently selected direction determines which checkbox appears. The inactive direction's saved setting is retained.

The shared **TTL mode** selector is shown only when the currently selected direction's **Enable cache** checkbox is checked. The numeric **cache TTL (ms)** field is shown only for **Fixed TTL**. Switching to **Adaptive TTL** hides the numeric field but retains its value as a fallback. Disabling the cache hides the TTL controls without erasing the settings. Only valid read functions 01–04 are cached; writes are not cached and trigger the documented invalidation rules.


> **Historical screenshots (v1.2.7):** They show the TCP/UDP selector and conventional cache options as they existed in that version. The current v1.3.2 source also includes the opt-in bounded TCP Client background-refresh controls described above.
