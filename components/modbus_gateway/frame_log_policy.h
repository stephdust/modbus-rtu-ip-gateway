#pragma once
// Fixed-size PSRAM frame log helpers, host-testable with a standard C++ compiler.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace frame_log_policy {
inline size_t encode_hex(char *out, size_t capacity,
                         const uint8_t *data, size_t length) {
  if (!out || capacity == 0U) return 0U;
  if (!data && length != 0U) { out[0] = '\0'; return 0U; }
  static constexpr char digits[] = "0123456789ABCDEF";
  // Three characters per byte (including separator). A size of 784
  // preserves every byte of a full 260-byte RTU frame and its terminator.
  const size_t max_bytes = (capacity - 1U) / 3U;
  const size_t count = length < max_bytes ? length : max_bytes;
  size_t pos = 0U;
  for (size_t i = 0U; i < count; ++i) {
    if (i) out[pos++] = ' ';
    out[pos++] = digits[data[i] >> 4U];
    out[pos++] = digits[data[i] & 0x0fU];
  }
  if (count != length && pos + 4U < capacity) {
    memcpy(out + pos, " ...", 4U);
    pos += 4U;
  }
  out[pos] = '\0';
  return pos;
}
} // namespace frame_log_policy
