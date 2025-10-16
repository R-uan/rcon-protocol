#pragma once

#include "packet.hpp"
#include <vector>

struct Protocol {
  void handle_packet(Packet packet);
  static std::vector<unsigned char> invalid_packet();
};
