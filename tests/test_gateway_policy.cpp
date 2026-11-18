// SPDX-License-Identifier: GPL-3.0-or-later
// g++ -std=c++11 -Wall -Wextra -Werror -pedantic
//   -Icomponents/modbus_gateway tests/test_gateway_policy.cpp -o /tmp/gateway_policy
// /tmp/gateway_policy
#include "modbus_gateway_policy.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace esphome::modbus_gateway;

struct CacheStub {
  bool valid;
  uint8_t direction;
  uint8_t unit_id;
  uint8_t function_code;
  uint16_t start_address;
  uint16_t quantity;
  uint32_t stored_at_ms;
  uint32_t ttl_ms;
};

static std::vector<uint8_t> rtu(uint8_t unit, const std::vector<uint8_t>& pdu) {
  std::vector<uint8_t> data(1, unit);
  data.insert(data.end(), pdu.begin(), pdu.end());
  const uint16_t crc = gateway_policy::crc16_rtu(data.data(), data.size());
  data.push_back(uint8_t(crc));
  data.push_back(uint8_t(crc >> 8));
  return data;
}

int main() {
  const uint8_t request03[] = {3, 0, 52, 0, 2};
  const std::vector<uint8_t> success03{3, 4, 0x12, 0x34, 0x56, 0x78};
  const std::vector<uint8_t> exception03{0x83, 2};
  const auto good = rtu(17, success03);
  assert(gateway_policy::valid_rtu_crc(good.data(), good.size()));
  assert(gateway_policy::classify_rtu_reply(17, request03, sizeof(request03),
                                          good.data(), good.size()) ==
         gateway_policy::RtuReply::Success);
  assert(gateway_policy::forward_original_rtu_reply(gateway_policy::RtuReply::Success, false));
  assert(gateway_policy::forward_original_rtu_reply(gateway_policy::RtuReply::Success, true));

  // A bad CRC must NEVER be cached or forwarded, regardless of error mode.
  auto bad_crc = good;
  bad_crc.back() ^= 0x01;
  assert(!gateway_policy::valid_rtu_crc(bad_crc.data(), bad_crc.size()));
  assert(gateway_policy::classify_rtu_reply(17, request03, sizeof(request03),
                                           bad_crc.data(), bad_crc.size()) ==
         gateway_policy::RtuReply::Invalid);
  assert(!gateway_policy::forward_original_rtu_reply(gateway_policy::RtuReply::Invalid, false));
  assert(!gateway_policy::forward_original_rtu_reply(gateway_policy::RtuReply::Invalid, true));

  // Even a CRC-correct but malformed/mismatched reply is NOT a valid result.
  const auto other_unit = rtu(18, success03);
  assert(gateway_policy::classify_rtu_reply(17, request03, sizeof(request03),
                                           other_unit.data(), other_unit.size()) ==
         gateway_policy::RtuReply::Invalid);
  const auto wrong_count = rtu(17, std::vector<uint8_t>{3, 2, 0x12, 0x34});
  assert(gateway_policy::classify_rtu_reply(17, request03, sizeof(request03),
                                           wrong_count.data(), wrong_count.size()) ==
         gateway_policy::RtuReply::Invalid);

  const auto exception = rtu(17, exception03);
  assert(gateway_policy::classify_rtu_reply(17, request03, sizeof(request03),
                                           exception.data(), exception.size()) ==
         gateway_policy::RtuReply::Exception);
  assert(gateway_policy::forward_original_rtu_reply(gateway_policy::RtuReply::Exception, false));
  assert(!gateway_policy::forward_original_rtu_reply(gateway_policy::RtuReply::Exception, true));

  auto bad_exception = exception;
  bad_exception[3] ^= 0x40;
  assert(gateway_policy::classify_rtu_reply(17, request03, sizeof(request03),
                                           bad_exception.data(), bad_exception.size()) ==
         gateway_policy::RtuReply::Invalid);

  // A write response must echo address/value; malformed write must never
  // invalidate the read cache.
  const uint8_t write06[] = {6, 0x03, 0xE9, 0x00, 0xFA};
  const uint8_t write_ok[] = {6, 0x03, 0xE9, 0x00, 0xFA};
  const uint8_t write_wrong[] = {6, 0x03, 0xE9, 0x00, 0xFB};
  assert(gateway_policy::is_success(write06, 5, write_ok, 5));
  assert(!gateway_policy::is_success(write06, 5, write_wrong, 5));

  // TTL fixed/adaptive expiry and unsigned clock rollover.
  assert(!gateway_policy::cache_expired(1499U, 1000U, 500U));
  assert(gateway_policy::cache_expired(1500U, 1000U, 500U));
  assert(!gateway_policy::cache_expired(9U, UINT32_MAX - 20U, 50U));
  assert(gateway_policy::cache_expired(30U, UINT32_MAX - 20U, 50U));

  // Cache: matching key MUST win over earlier vacant slot.
  CacheStub entries[4]{};
  entries[1] = {true, 1, 17, 3, 52, 97, 1000, 500};
  entries[2] = {true, 1, 17, 3, 52, 97, 1100, 500};
  auto* chosen = gateway_policy::cache_store_slot(
      entries, 4, 1200, 1, 17, 3, 52, 97, false, 500);
  assert(chosen == &entries[2]);  // newest matching key; NOT vacant [0]
  assert(!entries[1].valid);     // duplicate eliminated
  assert(entries[2].valid);

  // Fresh slot selected on expiry; an expired match must not be resurrected.
  auto* expired = gateway_policy::cache_store_slot(
      entries, 4, 1800, 1, 17, 3, 52, 97, false, 500);
  assert(expired != nullptr);
  assert(!entries[2].valid);

  // Adaptive expiry also rejects expired entries before reuse.
  entries[0] = {true, 1, 17, 3, 52, 97, 1000, 200};
  auto* adaptive = gateway_policy::cache_store_slot(
      entries, 4, 1201, 1, 17, 3, 52, 97, true, 500);
  assert(adaptive != nullptr);
  assert(!entries[0].valid);

  return 0;
}
