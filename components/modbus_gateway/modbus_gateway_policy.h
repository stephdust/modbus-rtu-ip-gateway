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

}  // namespace gateway_policy
}  // namespace modbus_gateway
}  // namespace esphome
