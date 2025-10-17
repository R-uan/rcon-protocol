#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

enum SERVERDATA {
  AUTH = 3,
  AUTH_RESPONSE = 2,
  EXECCOMAND = 2,
  RESPONSE_VALUE = 0
};

struct Packet {
  int size{-1};
  int id{-1};
  int type{-1};
  std::vector<char> data{};
};

Packet create_packet(const std::string_view data, int32_t id, int32_t type);

inline int u32_from_le(const std::vector<uint8_t> bytes) {
  return static_cast<int>(bytes[0] | bytes[1] << 8 | bytes[2] << 16 |
                          bytes[3] << 24);
}
