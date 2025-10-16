#pragma once

#include "packet.hpp"
#include <mutex>

struct ClientState {
  int fd;
  bool authenticated{false};

  std::mutex mtx;

  ClientState(int fd) : fd(fd) {}

  bool send_packet(Packet packet);
};
