// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Modbus RTU/IP Gateway contributors.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Transport-neutral Modbus TCP ADU codec. Modbus UDP uses the same MBAP
// envelope, but each UDP datagram must contain exactly one complete ADU.
namespace esphome {
namespace modbus_gateway {
namespace mbap {

static constexpr size_t HEADER_SIZE = 7;
static constexpr size_t MAX_PDU_SIZE = 253;
static constexpr size_t MAX_ADU_SIZE = HEADER_SIZE + MAX_PDU_SIZE;

struct Header {
  uint16_t transaction_id{0};
  uint16_t protocol_id{0};
  uint16_t length{0};  // Unit ID (1 byte) + PDU
  uint8_t unit_id{0};
};

inline uint16_t read_be16(const uint8_t *p) {
  return (uint16_t(p[0]) << 8) | p[1];
}

inline void write_be16(uint8_t *p, uint16_t value) {
  p[0] = uint8_t(value >> 8);
  p[1] = uint8_t(value);
}

// Reject truncated datagrams, concatenated ADUs, invalid protocol IDs,
// empty PDUs, and payloads larger than the Modbus application limit.
inline bool decode_datagram(const uint8_t *data, size_t size, Header &header,
                            const uint8_t *&pdu, size_t &pdu_size) {
  if (data == nullptr || size < HEADER_SIZE + 1 || size > MAX_ADU_SIZE)
    return false;
  Header parsed;
  parsed.transaction_id = read_be16(data);
  parsed.protocol_id = read_be16(data + 2);
  parsed.length = read_be16(data + 4);
  parsed.unit_id = data[6];
  if (parsed.protocol_id != 0 || parsed.length < 2 ||
      parsed.length > MAX_PDU_SIZE + 1 ||
      size != size_t(6) + parsed.length)
    return false;
  header = parsed;
  pdu = data + HEADER_SIZE;
  pdu_size = size - HEADER_SIZE;
  return true;
}

// Caller owns the output buffer. Returns 0 when arguments are invalid.
inline size_t encode_adu(uint8_t *output, size_t capacity,
                         uint16_t transaction_id, uint8_t unit_id,
                         const uint8_t *pdu, size_t pdu_size) {
  if (output == nullptr || pdu == nullptr || pdu_size == 0 ||
      pdu_size > MAX_PDU_SIZE || capacity < HEADER_SIZE + pdu_size)
    return 0;
  write_be16(output, transaction_id);
  write_be16(output + 2, 0);
  write_be16(output + 4, uint16_t(pdu_size + 1));
  output[6] = unit_id;
  memcpy(output + HEADER_SIZE, pdu, pdu_size);
  return HEADER_SIZE + pdu_size;
}

// For UDP client mode, the caller must ALSO verify the sender's IP address
// and UDP port against the configured endpoint before accepting a response.
inline bool matches_response(const Header &received, uint16_t transaction_id,
                             uint8_t unit_id) {
  return received.protocol_id == 0 && received.transaction_id == transaction_id &&
         received.unit_id == unit_id;
}

}  // namespace mbap
}  // namespace modbus_gateway
}  // namespace esphome
