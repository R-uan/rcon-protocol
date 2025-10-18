#pragma once

#include "client.hpp"
#include <memory>
#include <mutex>
#include <sys/epoll.h>
#include <unordered_map>

class RconServer {
private:
  int epollFd;
  int serverFd;
  std::mutex epollMtx;
  std::unordered_map<int, std::shared_ptr<Client>> clients;

  RconServer(int fd) : serverFd(fd) {
    this->epollFd = epoll_create1(0);
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = this->serverFd;
    epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->serverFd, &ev);
  }

  void add_client(int fd);
  void remove_client(int fd);
  int read_incoming(std::shared_ptr<Client> client);
  int read_packet_size(const std::shared_ptr<Client> client);

public:
  void listen();
  static RconServer create_instance(int port);
};
