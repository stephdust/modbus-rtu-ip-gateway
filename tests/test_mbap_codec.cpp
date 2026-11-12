// SPDX-License-Identifier: GPL-3.0-or-later
// Host-side smoke test: g++ -std=c++11 -Wall -Wextra -pedantic -Icomponents/modbus_gateway tests/test_mbap_codec.cpp -o /tmp/test_mbap && /tmp/test_mbap
#include "mbap_codec.h"
#include <cassert>
#include <cstdint>

using namespace esphome::modbus_gateway;

int main() {
  const uint8_t request[] = {0x03, 0x00, 0x34, 0x00, 0x02};
  uint8_t wire[mbap::MAX_ADU_SIZE] = {};
  const size_t n = mbap::encode_adu(wire, sizeof(wire), 0x1234, 11, request, sizeof(request));
  assert(n == 12);
  assert(wire[0] == 0x12 && wire[1] == 0x34);
  assert(wire[4] == 0 && wire[5] == 6 && wire[6] == 11);
  mbap::Header h;
  const uint8_t *pdu = nullptr;
  size_t pdu_size = 0;
  assert(mbap::decode_datagram(wire, n, h, pdu, pdu_size));
  assert(h.transaction_id == 0x1234 && h.unit_id == 11);
  assert(pdu_size == sizeof(request) && pdu[0] == 3);
  assert(mbap::matches_response(h, 0x1234, 11));
  assert(!mbap::matches_response(h, 0x1235, 11));
  assert(!mbap::matches_response(h, 0x1234, 12));
  assert(!mbap::decode_datagram(wire, n - 1, h, pdu, pdu_size));
  assert(!mbap::decode_datagram(wire, n + 1, h, pdu, pdu_size));
  wire[3] = 1;  // Invalid protocol ID
  assert(!mbap::decode_datagram(wire, n, h, pdu, pdu_size));
  wire[3] = 0;
  assert(mbap::encode_adu(wire, 4, 1, 1, request, sizeof(request)) == 0);
  assert(mbap::encode_adu(wire, sizeof(wire), 1, 1, request, 0) == 0);
  return 0;
}
