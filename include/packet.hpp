#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

// Packet Type: 32-bit little-endian integer, which indicates the purpose of the
// packet Its value will always be either 0, 2 or 3
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
