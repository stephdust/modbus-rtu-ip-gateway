// SPDX-License-Identifier: GPL-3.0-or-later
// g++ -std=c++11 -Wall -Wextra -Werror -pedantic
//   -Icomponents/modbus_gateway tests/test_tcp_connection_policy.cpp -o /tmp/tcp_connection_policy
// /tmp/tcp_connection_policy
#include "tcp_connection_policy.h"
#include <cassert>
#include <cstdint>

using namespace esphome::modbus_gateway::tcp_connection_policy;

int main() {
  assert(maintenance_allowed(true, true, true, true, true));
  assert(!maintenance_allowed(false, true, true, true, true)); // Server mode
  assert(!maintenance_allowed(true, false, true, true, true)); // UDP
  assert(!maintenance_allowed(true, true, false, true, true)); // One-shot TCP
  assert(!maintenance_allowed(true, true, true, false, true)); // RTU/TCP request in flight
  assert(!maintenance_allowed(true, true, true, true, false)); // Partial RTU frame
  assert(next_retry_delay(0) == 2000);
  assert(next_retry_delay(2000) == 4000);
  assert(next_retry_delay(4000) == 8000);
  assert(next_retry_delay(8000) == 16000);
  assert(next_retry_delay(16000) == 30000);
  assert(next_retry_delay(30000) == 30000);
  assert(due(1500, 1500));
  assert(!due(1499, 1500));
  // millis() roll-over while scheduling a short retry.
  assert(!due(0xFFFFFFF0U, 0x00000010U));
  assert(due(0x00000011U, 0x00000010U));
  assert(retry_wait_ms(0xFFFFFFF0U, 0x00000010U) == 32);
  assert(retry_wait_ms(0x00000011U, 0x00000010U) == 0);
}
