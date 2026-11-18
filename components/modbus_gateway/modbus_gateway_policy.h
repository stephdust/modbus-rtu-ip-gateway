// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Header-only Modbus response/TTL validation shared by the live gateway
// and host tests. No ESPHome/Arduino dependencies.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome {
namespace modbus_gateway {
namespace gateway_policy {

inline uint16_t word(const uint8_t* p) {
  return uint16_t(uint16_t(p[0]) << 8 | p[1]);
}

inline bool is_exception(const uint8_t* request, size_t request_len,
                         const uint8_t* response, size_t response_len) {
  return request && request_len && response && response_len == 2 &&
         response[0] == uint8_t(request[0] | 0x80);
}

inline bool is_success(const uint8_t* request, size_t request_len,
                       const uint8_t* response, size_t response_len) {
  if (!request || !request_len || !response || !response_len ||
      response[0] != request[0]) return false;
  switch (request[0]) {
    case 0x01: case 0x02: case 0x03: case 0x04: {
      if (request_len != 5 || response_len < 2) return false;
      const uint16_t count = word(request + 3);
      if (!count || count > ((request[0] <= 2) ? 2000U : 125U)) return false;
      const size_t bytes = (request[0] <= 2) ? (count + 7U) / 8U : count * 2U;
      return response[1] == bytes && response_len == bytes + 2U;
    }
    case 0x05: case 0x06:
      return request_len == 5 && response_len == 5 &&
             memcmp(request, response, 5) == 0;
    case 0x0F: case 0x10: {
      if (request_len < 6 || response_len != 5) return false;
      const uint16_t count = word(request + 3);
      const size_t bytes = request[0] == 0x0F ? (count + 7U) / 8U : count * 2U;
      if (!count || count > (request[0] == 0x0F ? 1968U : 123U) ||
          request[5] != bytes || request_len != 6U + bytes) return false;
      return memcmp(request, response, 5) == 0;
    }
    case 0x17: {
      if (request_len < 10 || response_len < 2) return false;
      const uint16_t reads = word(request + 3);
      const uint16_t writes = word(request + 7);
      if (!reads || reads > 125U || !writes || writes > 121U ||
          request[9] != writes * 2U ||
          request_len != 10U + writes * 2U) return false;
      return response[1] == reads * 2U &&
             response_len == 2U + reads * 2U;
    }
    default:
      return false;
  }
}

inline bool cache_expired(uint32_t now_ms, uint32_t stored_at_ms, uint32_t ttl_ms) {
  return uint32_t(now_ms - stored_at_ms) >= ttl_ms;
}

// Select the newest existing entry for a key BEFORE considering a free
// slot. This also removes duplicate keys left by earlier firmware versions.
// Entries hold only POD metadata, so this is allocation-free and host-testable.
template <typename Entry>
inline Entry* cache_store_slot(Entry* entries, size_t count, uint32_t now_ms,
                               uint8_t direction, uint8_t unit_id,
                               uint8_t function_code, uint16_t address,
                               uint16_t quantity, bool adaptive,
                               uint32_t fixed_ttl_ms) {
  if (!entries) return nullptr;
  Entry* matching = nullptr;
  Entry* vacant = nullptr;
  Entry* oldest = nullptr;
  uint32_t oldest_age = 0;
  for (size_t i = 0; i < count; ++i) {
    Entry& entry = entries[i];
    if (entry.valid && cache_expired(
            now_ms, entry.stored_at_ms,
            adaptive ? entry.ttl_ms : fixed_ttl_ms))
      entry.valid = false;
    if (entry.valid && entry.direction == direction && entry.unit_id == unit_id &&
        entry.function_code == function_code && entry.start_address == address &&
        entry.quantity == quantity) {
      // If old firmware created duplicates, keep the freshest response.
      if (!matching ||
          uint32_t(now_ms - entry.stored_at_ms) <
              uint32_t(now_ms - matching->stored_at_ms)) {
        if (matching) matching->valid = false;
        matching = &entry;
      } else entry.valid = false;
      continue;
    }
    if (!entry.valid && !vacant) vacant = &entry;
    if (entry.valid) {
      const uint32_t age = uint32_t(now_ms - entry.stored_at_ms);
      if (!oldest || age > oldest_age) { oldest = &entry; oldest_age = age; }
    }
  }
  return matching ? matching : (vacant ? vacant : oldest);
}

}  // namespace gateway_policy
}  // namespace modbus_gateway
}  // namespace esphome
