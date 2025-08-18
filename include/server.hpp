#pragma once

#include "client.hpp"
#include <mutex>
#include <thread>
#include <unordered_map>

struct Server {
  int port;
  int server_fd;

  std::unordered_map<int, Client> clients;

  bool listen_state;
  std::thread listening_thread;
  

  mutable std::mutex fd_mutex;
  mutable std::mutex listen_mx;  

  Server(int port, int server_fd) :
    server_fd(server_fd),
    port(port),
    listening_thread(nullptr),
    listen_state(false)
    { }

  static Server create_instance(int port);
  void open_server();
  void listen();
};
