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

// An RTU reply MUST pass CRC, Unit ID, function and PDU shape validation
// before the gateway may forward it or place it in the read cache.
// TCP/UDP Modbus ADUs do not contain a Modbus RTU CRC.
inline uint16_t crc16_rtu(const uint8_t* data, size_t size) {
  uint16_t crc = 0xFFFFU;
  if (!data) return 0U;
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8U; ++bit)
      crc = (crc & 1U) ? uint16_t((crc >> 1U) ^ 0xA001U) : uint16_t(crc >> 1U);
  }
  return crc;
}

inline bool valid_rtu_crc(const uint8_t* frame, size_t size) {
  if (!frame || size < 4U) return false;
  const uint16_t found = uint16_t(frame[size - 2U]) |
                         uint16_t(uint16_t(frame[size - 1U]) << 8U);
  return found == crc16_rtu(frame, size - 2U);
}

enum class RtuReply : uint8_t { Invalid, Success, Exception };

inline RtuReply classify_rtu_reply(uint8_t expected_unit,
                                   const uint8_t* request_pdu, size_t request_size,
                                   const uint8_t* frame, size_t frame_size) {
  if (!request_pdu || !request_size || !frame || frame_size < 5U ||
      frame[0] != expected_unit || !valid_rtu_crc(frame, frame_size))
    return RtuReply::Invalid;
  const uint8_t* response_pdu = frame + 1U;
  const size_t response_size = frame_size - 3U;
  if (is_success(request_pdu, request_size, response_pdu, response_size))
    return RtuReply::Success;
  if (is_exception(request_pdu, request_size, response_pdu, response_size))
    return RtuReply::Exception;
  return RtuReply::Invalid;
}

// An exception may be newly generated for an INVALID response in standard
// mode, but the invalid raw RTU frame must never be forwarded or cached.
inline bool forward_original_rtu_reply(RtuReply kind, bool silent) {
  return kind == RtuReply::Success ||
         (kind == RtuReply::Exception && !silent);
}

inline bool cache_expired(uint32_t now_ms, uint32_t stored_at_ms, uint32_t ttl_ms) {
  return uint32_t(now_ms - stored_at_ms) >= ttl_ms;
}

// SWR never extends the lifetime indefinitely: the fixed maximum age is
// measured from the LAST validated network response, not from cache hits.
// An adaptive fresh TTL may exceed the configured stale limit, in which case
// the larger fresh TTL wins until that entry actually becomes stale.
enum class CacheAge : uint8_t { Unavailable, Fresh, Stale };
inline CacheAge classify_cache_age(uint32_t now_ms, uint32_t stored_ms,
                                    uint32_t fresh_ttl_ms,
                                    uint32_t stale_max_age_ms,
                                    bool allow_stale) {
  const uint32_t retained_ms = allow_stale && stale_max_age_ms > fresh_ttl_ms
      ? stale_max_age_ms : fresh_ttl_ms;
  if (cache_expired(now_ms, stored_ms, retained_ms)) return CacheAge::Unavailable;
  return cache_expired(now_ms, stored_ms, fresh_ttl_ms)
      ? CacheAge::Stale : CacheAge::Fresh;
}

// Before transmitting a non-background RTU reply, ensure that its complete
// on-wire duration (including direction switching guard) fits the configured
// master-response window. Uses 64-bit arithmetic for long frames/slow baud.
inline bool rtu_reply_fits_window(uint32_t elapsed_us, size_t frame_bytes,
                                   uint32_t baud, uint8_t bits_per_char,
                                   uint32_t window_ms, uint32_t guard_ms = 20U) {
  if (!baud || !bits_per_char || !frame_bytes || !window_ms) return false;
  const uint64_t tx_us = (uint64_t(frame_bytes) * bits_per_char * 1000000ULL
                         + baud - 1U) / baud;
  return uint64_t(elapsed_us) + tx_us + uint64_t(guard_ms) * 1000ULL
         <= uint64_t(window_ms) * 1000ULL;
}

// Select the newest existing entry for a key BEFORE considering a free
// slot. This also removes duplicate keys left by earlier firmware versions.
// Entries hold only POD metadata, so this is allocation-free and host-testable.
template <typename Entry>
inline Entry* cache_store_slot(Entry* entries, size_t count, uint32_t now_ms,
                               uint8_t direction, uint8_t unit_id,
                               uint8_t function_code, uint16_t address,
                               uint16_t quantity, bool adaptive,
                               uint32_t fixed_ttl_ms,
                               uint32_t stale_retention_ms = 0U) {
  if (!entries) return nullptr;
  Entry* matching = nullptr;
  Entry* vacant = nullptr;
  Entry* oldest = nullptr;
  uint32_t oldest_age = 0;
  for (size_t i = 0; i < count; ++i) {
    Entry& entry = entries[i];
    if (entry.valid) {
      const uint32_t fresh_ttl = adaptive ? entry.ttl_ms : fixed_ttl_ms;
      const uint32_t retention = (entry.direction == direction &&
          direction == 0U && stale_retention_ms > fresh_ttl)
          ? stale_retention_ms : fresh_ttl;
      if (cache_expired(now_ms, entry.stored_at_ms, retention))
        entry.valid = false;
    }
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
