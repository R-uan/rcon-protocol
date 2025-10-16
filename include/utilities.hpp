#pragma once

#include <cstdint>
#include <vector>

inline int u32_from_le(const std::vector<uint8_t> bytes) {
  return static_cast<int>(bytes[0] | bytes[1] << 8 | bytes[2] << 16 |
                          bytes[3] << 24);
}
