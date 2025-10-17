#include "client.hpp"
#include <iostream>
#include <sys/socket.h>

bool Client::send_packet(Packet packet) {
  const auto data = packet.data.data();
  if (send(this->fd, data, sizeof(data), 0) == -1) {
    std::cerr << "Failed to send data to client" << std::endl;
    return false;
  }
  std::cout << "Packet was sent to client" << std::endl;
  return true;
}
