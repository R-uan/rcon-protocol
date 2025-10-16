#include "client.hpp"
#include <iostream>
#include <sys/socket.h>

bool ClientState::send_packet(Packet packet) {
  const auto data = packet.data.data();
  if (send(this->fd, data, sizeof(data), 0) == -1) {
    std::cerr << "Failed to send data to client" << std::endl;
    return false;
  }
  return true;
}
