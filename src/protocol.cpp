#include "protocol.hpp"
#include <vector>

std::vector<unsigned char> Protocol::invalid_packet() {
  return std::vector<unsigned char>(0x00);
}
