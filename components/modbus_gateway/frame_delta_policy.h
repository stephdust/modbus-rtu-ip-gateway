#pragma once
// Host-testable selection for the gateway's incremental Web frame feed.
// Does not depend on Arduino, ESPHome, Wi-Fi or runtime heap allocations.
#include <stddef.h>
#include <stdint.h>

namespace frame_delta_policy {
struct Selection {
  size_t count;
  bool reset;
};

inline size_t visible_count(size_t retained, size_t limit) {
  return retained < limit ? retained : limit;
}

inline Selection select(uint32_t newest, size_t retained, size_t limit,
                        bool has_cursor, uint32_t after, bool same_epoch) {
  const size_t visible = visible_count(retained, limit);
  if (!has_cursor || !same_epoch) return {visible, true};
  const uint32_t difference = uint32_t(newest - after);
  if (difference == 0U) return {0U, false};
  // A stale/future cursor or more messages than the display retains requires
  // a fresh snapshot. This also protects against an epoch/cursor anomaly.
  if (difference > 0x7fffffffU || difference > visible) return {visible, true};
  return {size_t(difference), false};
}

inline bool parse_u32(const char *text, uint32_t &out) {
  if (!text || !*text) return false;
  uint32_t result = 0U;
  for (const char *p = text; *p; ++p) {
    if (*p < '0' || *p > '9') return false;
    const uint32_t digit = uint32_t(*p - '0');
    if (result > (UINT32_MAX - digit) / 10U) return false;
    result = result * 10U + digit;
  }
  out = result;
  return true;
}
} // namespace frame_delta_policy
