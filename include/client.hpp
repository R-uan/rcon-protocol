#pragma once

#include "utilities.hpp"
#include <atomic>
#include <mutex>

struct Client {
  int fd;
  std::mutex mtx;
  bool authenticated{false};
  std::atomic<int> commandsExec{0};

  Client(int fd) : fd(fd) {}

  bool send_packet(Packet packet);
};
