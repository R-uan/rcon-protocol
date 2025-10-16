#include "client.hpp"
#include "packet.hpp"
#include "server.hpp"
#include "thread_pool.hpp"
#include <arpa/inet.h>
#include <bits/socket.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <utility>

RconServer RconServer::create_instance(int port) {
  const int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    std::cerr << "Could not create server socket" << std::endl;
    throw new std::system_error();
  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (bind(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    std::cerr << "Server could not be initialized on given addr" << std::endl;
    close(fd);
    throw new std::system_error();
  }

  if (::listen(fd, SOMAXCONN) == -1) {
    std::cerr << "Listen failed" << std::endl;
    close(fd);
    throw std::system_error(errno, std::generic_category());
  }

  std::cout << "Server connected" << std::endl;
  return RconServer(fd);
}

void RconServer::listen() {
  epoll_event events[50];
  ThreadPool threadPool(10);
  while (true) {
    int nfds = epoll_wait(this->epollFd, events, 50, -1);
    for (int i = 0; i < nfds; i++) {
      int fd = events[i].data.fd;
      if (fd == this->serverFd) {
        int newClientFd = accept(serverFd, nullptr, nullptr);
        if (newClientFd != -1) {
          std::cout << "New client connected" << std::endl;
          this->add_client(newClientFd);
        }
      } else {
        char buffer[4096];
        ssize_t readb = recv(fd, buffer, sizeof(buffer), 0);

        // Look for the ClientState associated with this fd.
        auto it = this->clients.find(fd);
        if (it == this->clients.end())
          continue;
        auto client_ptr = it->second.get();

        if (readb <= 0) {
          // if no bytes is read the client is discarded.
          this->remove_client(fd);
          continue;
        }

        auto packet = Packet::parse_bytes(buffer, readb);
        // close connection in case of invalid packet
        if (!packet.has_value()) {
          this->remove_client(fd);
          continue;
        }

        threadPool.enqueue([client_ptr, packet]() {
          client_ptr->handle_packet(packet.value());
        });
      }
    }
  }
}

void RconServer::add_client(int fd) {
  epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLIN;
  {
    std::unique_lock lock(this->epollMtx);
    epoll_ctl(this->epollFd, EPOLL_CTL_ADD, fd, &ev);
  }
  auto client = std::make_shared<ClientState>(fd);
  this->clients[fd] = std::move(client);
}

void RconServer::remove_client(int fd) {
  {
    std::unique_lock lock(this->epollMtx);
    epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, nullptr);
  }
  this->clients.erase(fd);
  close(fd);
}
