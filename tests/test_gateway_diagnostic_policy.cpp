// Compile: g++ -std=c++17 -Wall -Wextra -Werror -pedantic \
//   tests/test_gateway_diagnostic_policy.cpp -o /tmp/gateway_diag_policy && /tmp/gateway_diag_policy
#include <cassert>
#include <cstdint>
#include "../components/modbus_gateway/gateway_diagnostic_policy.h"
using namespace gateway_diagnostic_policy;
int main() {
  assert(rtu_wire_time_us(197U, 9600U, 10U) >= 205000U);
  assert(rtu_wire_time_us(197U, 9600U, 10U) <= 206000U);
  assert(!unexpected_uart_tx(215000U, rtu_wire_time_us(197U, 9600U, 10U)));
  assert(unexpected_uart_tx(340000U, rtu_wire_time_us(197U, 9600U, 10U)));
  assert(likely_uart_buffered(130U, rtu_wire_time_us(7U, 9600U, 10U)));
  assert(!likely_uart_buffered(7800U, rtu_wire_time_us(7U, 9600U, 10U)));
  assert(!likely_uart_buffered(0U, 0U));
  assert(!noteworthy_loop_gap(100000U));
  assert(noteworthy_loop_gap(500000U));
  assert(interval_elapsed(100U, 0U, 3000U));
  assert(!interval_elapsed(2999U, 1000U, 3000U));
  assert(interval_elapsed(5000U, 1000U, 3000U));
  assert(interval_elapsed(0x00000010U, 0xfffff000U, 3000U));
  assert(!interval_elapsed(0x00000010U, 0xfffffff0U, 3000U));
  for (uint32_t us=0; us<=1000000U; us+=1000U) {
    assert(unexpected_uart_tx(us, 200000U) == (us>280000U));
    assert(noteworthy_loop_gap(us) == (us>=250000U));
  }
  return 0;
}
