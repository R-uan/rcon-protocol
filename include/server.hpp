#pragma once

#include "client.hpp"
#include "utilities.hpp"
#include <memory>
#include <mutex>
#include <sys/epoll.h>
#include <unordered_map>

class RconServer {
private:
  int epollFd;
  int serverFd;
  Logger logger;
  std::mutex epollMtx;
  std::unordered_map<int, std::shared_ptr<Client>> clients;

  void add_client(int fd);
  void remove_client(std::shared_ptr<Client> client);
  int read_incoming(std::shared_ptr<Client> client);
  int read_packet_size(const std::shared_ptr<Client> client);

  RconServer(int fd) : serverFd(fd), logger(Logger()) {
    this->epollFd = epoll_create1(0);
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = this->serverFd;
    epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->serverFd, &ev);
  }

public:
  void listen();
  ~RconServer() { this->logger.stop(); }
  static RconServer create_instance(int port);
};
