#include "client.hpp"
#include <sys/socket.h>

bool Client::send_packet(Packet packet) {
  const auto data = packet.data.data();
  auto sent = send(this->fd, data, sizeof(data), 0);
  if (sent == -1) {
    return false;
  }
  return true;
}
