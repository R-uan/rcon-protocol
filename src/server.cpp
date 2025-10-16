#include "client.hpp"
#include "packet.hpp"
#include "server.hpp"
#include "thread_pool.hpp"
#include "utilities.hpp"
#include <arpa/inet.h>
#include <bits/socket.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

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
        auto it = this->clients.find(fd);
        if (it == this->clients.end())
          continue;
        std::shared_ptr<ClientState> client_ptr = it->second;
        threadPool.enqueue(
            [this, client_ptr]() { this->read_incoming(client_ptr); });
      }
    }
  }
}

int read_packet_size(const std::shared_ptr<ClientState> client) {
  std::vector<uint8_t> buff{};
  buff.resize(4);

  if (recv(client->fd, buff.data(), 4, 0) == -1) {
    std::cerr << "Could not read packet" << std::endl;
    return -1;
  }

  return u32_from_le(buff);
}

void RconServer::read_incoming(std::shared_ptr<ClientState> client) {
  size_t packetSize = read_packet_size(client);
  if (packetSize <= 10)
    return;

  std::vector<uint8_t> buffer{};
  buffer.resize(packetSize);

  if (recv(client->fd, buffer.data(), packetSize, 0) == -1) {
    std::cerr << "Could not get packet from client" << std::endl;
    return;
  }

  // Skips the first 8 bytes (id and type) and the last 2 (null bytes)
  std::string packetBody(&buffer[8], &buffer[buffer.size() - 2]);
  int pid = u32_from_le({buffer[0], buffer[1], buffer[2], buffer[3]});
  int type = u32_from_le({buffer[4], buffer[5], buffer[6], buffer[7]});

  Packet responsePacket{};

  if (!client->authenticated) {
    if (packetBody == "password") {
      std::cout << "Client authenticated" << std::endl;
      responsePacket = create_packet("", pid, SERVERDATA::AUTH_RESPONSE);
    } else {
      std::cout << "Authentication failed" << std::endl;
      responsePacket = create_packet("", -1, SERVERDATA::AUTH_RESPONSE);
    }
  }

  if (client->send_packet(responsePacket)) {
    std::cout << "Packet was sent to client" << std::endl;
    return;
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
