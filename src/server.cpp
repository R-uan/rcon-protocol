#include "server.hpp"
#include <arpa/inet.h>
#include <bits/socket.h>
#include <mutex>
#include <sys/socket.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <sys/epoll.h>

Server Server::create_instance(int port) {
  const int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) { throw new std::system_error(); }

  sockaddr_in serverAddr {};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
    throw new std::system_error();
    close(fd);
  }

  return Server(port, fd);
}

void Server::listen() {
  std::lock_guard<std::mutex> lock(this->listen_mx);
  this->listen_state = true;
  this->listening_thread = std::thread(&Server::open_server, this);
}

void Server::open_server() {
  int epfd = epoll_create1(0);
  // 向poll添加一个FD
  epoll_event serverev;
  serverev.events = EPOLLIN;
  serverev.data.fd = this->server_fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, this->server_fd, &serverev);
  // max event number
  epoll_event events[50];
  while(this->listen_state) {
    int nfds = epoll_wait(epfd, events, 50, -1);
    for (int i = 0; i < nfds; i++) {
      int fd = events[i].data.fd;
    }
  }
}
