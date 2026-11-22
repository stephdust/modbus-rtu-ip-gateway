// SPDX-License-Identifier: GPL-3.0-or-later
// Pure scheduling policy for the persistent TCP client supervisor (C++11).
#pragma once
#include <stdint.h>

namespace esphome {
namespace modbus_gateway {
namespace tcp_connection_policy {

static constexpr uint32_t kPollIntervalMs = 1000U;
static constexpr uint32_t kInitialRetryMs = 2000U;
static constexpr uint32_t kMaximumRetryMs = 30000U;
static constexpr int32_t kIdleConnectTimeoutMs = 250;

// ESPHome's software connection state is useful as a diagnostic, but an
// ESP32 station should be considered usable for TCP only after the Wi-Fi
// driver reports association AND the STA interface has an IPv4 address.
// In particular, the fallback softAP IP is not a station address.
inline bool station_link_ready(bool station_associated, bool station_has_ipv4) {
  return station_associated && station_has_ipv4;
}

inline bool status_disagrees(bool esphome_connected, bool station_associated,
                             bool station_has_ipv4) {
  return esphome_connected != station_link_ready(station_associated, station_has_ipv4);
}


inline bool due(uint32_t now_ms, uint32_t deadline_ms) {
  // Supports millis() wraparound as long as intervals are below 2^31 ms.
  return static_cast<int32_t>(now_ms - deadline_ms) >= 0;
}

inline uint32_t next_retry_delay(uint32_t current_ms) {
  if (current_ms < kInitialRetryMs) return kInitialRetryMs;
  if (current_ms >= kMaximumRetryMs / 2U) return kMaximumRetryMs;
  return current_ms * 2U;
}

inline bool maintenance_allowed(bool client_mode, bool tcp_transport,
                                bool persistent, bool transaction_idle,
                                bool no_pending_rtu_bytes) {
  // Never open a background socket in Server/UDP/non-persistent mode or
  // while forwarding an RTU transaction or receiving a partial RTU frame.
  return client_mode && tcp_transport && persistent &&
         transaction_idle && no_pending_rtu_bytes;
}

inline uint32_t retry_wait_ms(uint32_t now_ms, uint32_t deadline_ms) {
  return due(now_ms, deadline_ms) ? 0U : deadline_ms - now_ms;
}

}  // namespace tcp_connection_policy
}  // namespace modbus_gateway
}  // namespace esphome
