#pragma once
// No Arduino, lwIP or ESPHome dependency: pure RTU/TCP arbitration policy.
#include <stddef.h>
#include <stdint.h>
namespace deferred_rtu_policy {
static constexpr size_t kMaxModbusPdu = 253U;
inline bool is_read(uint8_t function) {
  return function >= 1U && function <= 4U;
}
inline bool should_defer(bool active, bool background_refresh,
                         bool cache_hit, uint8_t function) {
  // Never defer writes or mixed read/write functions.
  return active && background_refresh && !cache_hit && is_read(function);
}
inline bool read_expired(uint8_t function, uint32_t age_us, uint32_t window_ms) {
  return is_read(function) && uint32_t(age_us / 1000U) >= window_ms;
}
inline bool discard_for_new_master(uint8_t function, bool master_busy) {
  return is_read(function) && master_busy;
}
inline bool can_dispatch(bool pending, bool idle) { return pending && idle; }
} // namespace deferred_rtu_policy
