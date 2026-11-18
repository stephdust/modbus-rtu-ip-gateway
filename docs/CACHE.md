# Modbus response cache

The gateway includes an optional response cache for each operating mode. The cache is intended to reduce repeated polling latency when a master asks for the same read data more than once within a short interval.

## Independent mode switches

The Web UI exposes two independent settings in **1. Modbus IP**. To keep the page unambiguous, it displays only the setting that belongs to the currently selected Client/Server mode:

- **IP Client selected:** the UI shows **Enable cache in Client IP mode (RTU → IP)**, which reuses a previously received Modbus TCP read response for a matching RTU request.
- **IP Server selected:** the UI shows **Enable cache in Server IP mode (IP → RTU)**, which reuses a previously received RTU read response for a matching TCP request.

Both settings are independent and disabled by default. Changing the selected Client/Server mode updates the visible switch immediately without reloading the page. Saving in one mode updates only that mode’s cache setting; the hidden mode’s setting is preserved in NVS and appears again when that mode is selected.

## Web UI behavior (v1.2.6)

In **TCP Client** or **UDP Client**, only the **Client cache** switch is shown; in **TCP Server** or **UDP Server**, only the **Server cache** switch is shown. The inactive direction's saved setting is preserved. The **TTL mode** selector appears only when the selected direction's cache is enabled; the numeric TTL input appears only for Fixed TTL.

For v1.2.2 UI simulations (not real screenshots), see the [four-mode interface gallery](QUICK_START.md#aperçus-de-linterface-web-v121--simulations).

## TTL modes: fixed or adaptive (v1.2.6)

The Web interface offers a **TTL mode** selector whenever the active Client/Server cache is enabled:

- **Fixed TTL** (default, backward compatible): manually enter a TTL from 50 to 10,000 ms (default 500 ms).
- **Adaptive TTL**: estimate the polling interval separately for each cache key (direction, Unit ID, function, start address and quantity), then choose a TTL of 80% of the observed interval, bounded to 50–10,000 ms. Until an interval has been observed, the configured fixed TTL is used as a fallback.

This feature is included in the v1.2.6 source and still requires compilation and hardware validation. The selected TTL mode is saved in ESP32 NVS. Adaptive estimates are held only in RAM and reset when the cache is cleared. The age of a cached response is always measured from the time it was fetched, never from a cache hit. Writes continue to invalidate affected cached reads.

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

## Data freshness warning

A cache hit intentionally returns a previous valid device response. The data can be as old as the applicable fixed or adaptive TTL. Choose a TTL appropriate for the process and its update rate. If stale data is unsafe or unacceptable, leave the corresponding mode's cache disabled. The cache is a performance option, not a substitute for device-side update timing or application-level freshness checks.

## Interaction with asynchronous Client TCP processing

From component release 1.1.9, normal RTU-originated Client TCP transactions receive the remote TCP response cooperatively through the ESPHome loop. Cache hits remain immediate and do not open or use the TCP socket. A manual transaction initiated from the Web UI still uses the synchronous diagnostic helper; it is refused with HTTP 409 while a normal Client TCP transaction is already in progress so that both operations cannot read from the same TCP stream concurrently.

## TCP and UDP (v1.2.6)

Both cache switches are **direction-specific**, not transport-specific: **Client** applies to TCP Client and UDP Client, and **Server** applies to TCP Server and UDP Server. The currently selected direction determines which checkbox appears. The inactive direction's saved setting is retained.

The shared **TTL mode** selector is shown only when the currently selected direction's **Enable cache** checkbox is checked. The numeric **cache TTL (ms)** field is shown only for **Fixed TTL**. Switching to **Adaptive TTL** hides the numeric field but retains its value as a fallback. Disabling the cache hides the TTL controls without erasing the settings. Only valid read functions 01–04 are cached; writes are not cached and trigger the documented invalidation rules.
