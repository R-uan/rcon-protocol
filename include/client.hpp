#pragma once

#include "utilities.hpp"
#include <mutex>

struct Client {
  int fd;
  std::mutex mtx;
  bool authenticated{false};

  Client(int fd) : fd(fd) {}

  bool send_packet(Packet packet);
};
