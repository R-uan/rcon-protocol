#pragma once

#include "packet.hpp"

struct Protocol {
  void handle_packet(Packet packet);
};
