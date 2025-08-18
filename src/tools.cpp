#include "tools.hpp"
#include <string>
#include <vector>

std::vector<unsigned char> string_to_bytes(std::string &str) {
  std::vector<unsigned char> bytes(str.begin(), str.end());
  bytes.push_back(0x00);
  return bytes;
}

void int_to_bytes(int32_t value, unsigned char *bytes) {
  bytes[0] = value & 0xFF;
  bytes[1] = (value >> 8) & 0xFF;
  bytes[2] = (value >> 16) & 0xFF;
  bytes[3] = (value >> 24) & 0xFF;
}
