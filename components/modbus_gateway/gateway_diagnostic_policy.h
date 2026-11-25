#pragma once
// Diagnostic-only arithmetic: no Arduino/ESPHome dependency, no allocation,
// UART operation or socket side effect. Host-testable before firmware builds.
#include <stddef.h>
#include <stdint.h>

namespace gateway_diagnostic_policy {

inline uint32_t rtu_wire_time_us(size_t bytes, uint32_t baud, uint32_t bits_per_char) {
  if (!baud || !bits_per_char) return 0U;
  return uint32_t((uint64_t(bytes) * uint64_t(bits_per_char) * 1000000ULL) / baud);
}
// Normal transmission includes wire time, DE/RE switching, driver overhead.
// No false positives for a 97-register response at 9600 baud (~210 ms).
inline bool unexpected_uart_tx(uint32_t elapsed_us, uint32_t wire_us) {
  return elapsed_us > wire_us + 80000U;
}
// If all frame bytes were already sitting in the UART FIFO before our loop
// read them, measured first-to-last is much shorter than physical wire time.
inline bool likely_uart_buffered(uint32_t measured_us, uint32_t wire_us) {
  return wire_us > 0U && measured_us < wire_us / 3U;
}
inline bool interval_elapsed(uint32_t now_ms, uint32_t last_ms, uint32_t interval_ms) {
  return last_ms == 0U || uint32_t(now_ms - last_ms) >= interval_ms;
}
inline bool noteworthy_loop_gap(uint32_t gap_us) { return gap_us >= 250000U; }

}  // namespace gateway_diagnostic_policy
