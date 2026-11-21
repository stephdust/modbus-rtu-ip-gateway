# v1.3.8 — Persistent TCP Client automatic reconnect

Validated by the project maintainer on ESP32-S3 hardware (21 November 2026).

## What changed

- Persistent TCP Client sockets are monitored **even with no incoming Modbus RTU frames** and with the Web frame/statistics view closed.
- TCP keepalive is configured when supported (15-second idle, 5-second probe interval, three probes).
- A disconnected persistent TCP client automatically reconnects with progressive 2–30-second retry delays.
- Supervision stays active when Web RTU frame listening is paused. No synthetic Modbus traffic is generated.
- The client does not reconnect during an in-flight transaction or while a partial RTU frame is buffered.
- Existing single retry for read functions 03/04 remains; writes are never automatically replayed.
- New `/connections` diagnostic fields report the supervisor state, reconnect attempts/results, idle disconnects and keepalive setup.
- Host policy tests cover eligibility, retry backoff and millis() rollover.

## Compatibility

- ESPHome, Arduino framework and ESP32-S3, including Waveshare ESP32-S3-RS485-CAN.
- Existing TCP Server, UDP Client and UDP Server behavior is intended to remain unchanged.
- TCP connection persistence remains optional and is used only in TCP Client mode.
- Existing configuration is preserved.

## Validation and limits

- **Maintainer-reported hardware result:** the corrected gateway works in the physical installation.
- C++11 connection-policy host assertions pass.
- Independent overnight soak, complete TCP/UDP regression and ESPHome build in this environment were not performed. Do not claim comprehensive reliability certification.

## Install in ESPHome / Home Assistant

After the GitHub tag is published, set the YAML external component to:

```yaml
external_components:
  - source: github://stephdust/my-modbus-rtu-ip-gateway@v1.3.8
    components: [modbus_gateway]
```

Then compile/install through ESPHome. Configure your Wi-Fi and existing gateway settings as before.

See `docs/TCP_IDLE_RECONNECT.md` for complete behavior and troubleshooting.
