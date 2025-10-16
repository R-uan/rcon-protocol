#pragma once

#include "packet.hpp"
#include <mutex>

struct ClientState {
  int client_fd;
  bool authenticated {false};

  std::mutex mtx;
  
  ClientState(int fd)  : client_fd(fd) { }

  void handle_packet(Packet packet);
  void send_packet(Packet packet);
};
