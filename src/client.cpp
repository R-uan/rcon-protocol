#include "client.hpp"
#include <iostream>
#include <sys/socket.h>

bool Client::send_packet(Packet packet) {
  const auto data = packet.data.data();
  auto sent = send(this->fd, data, sizeof(data), 0);
  if (sent == -1) {
    std::cerr << "[CLIENT] failed to send data to client" << std::endl;
    return false;
  }
  std::cout << "[CLIENT] sent " << sent << " bytes" << std::endl;
  return true;
}
