#include "client.hpp"
#include "commands.hpp"
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
          std::cout << "[SERVER] new client connected" << std::endl;
          this->add_client(newClientFd);
        }
      } else {
        std::cout << "[SERVER] new entry from " << fd << std::endl;
        auto it = this->clients.find(fd);
        if (it == this->clients.end())
          continue;
        std::shared_ptr<Client> client_ptr = it->second;

        // epoll_event ev;
        // ev.data.fd = fd;
        // ev.events = 0;
        // {
        //   std::unique_lock lock(this->epollMtx);
        //   epoll_ctl(this->epollFd, EPOLL_CTL_MOD, fd, &ev);
        // }

        threadPool.enqueue([this, client_ptr]() {
          int status = this->read_incoming(client_ptr);
          if (status == 0) {
            epoll_event ev;
            ev.data.fd = client_ptr->fd;
            ev.events = EPOLLIN | EPOLLONESHOT;
            {
              std::unique_lock lock(this->epollMtx);
              epoll_ctl(this->epollFd, EPOLL_CTL_MOD, client_ptr->fd, &ev);
            }
          }
        });
      }
    }
  }
}

int RconServer::read_packet_size(const std::shared_ptr<Client> client) {
  std::vector<uint8_t> buff{};
  buff.resize(4);

  ssize_t n = recv(client->fd, buff.data(), 4, 0);
  if (n == 0) {
    // Client closed connection
    std::cerr << "[CLIENT] connection closed by peer" << std::endl;
    this->remove_client(client->fd);
    return -1;
  }
  if (n < 0) {
    std::cerr << "[CLIENT] could not read packet size" << std::endl;
    this->remove_client(client->fd);
    return -1;
  }

  return u32_from_le(buff);
}

int RconServer::read_incoming(std::shared_ptr<Client> client) {
  std::unique_lock lock(client->mtx);

  int packetSize = read_packet_size(client);

  if (packetSize <= 10) {
    return -1;
  }

  std::cout << "[CLIENT] read " << packetSize << " bytes" << std::endl;

  std::vector<uint8_t> buffer{};
  buffer.resize(packetSize);

  if (recv(client->fd, buffer.data(), packetSize, 0) == -1) {
    std::cerr << "[CLIENT] could not read packet" << std::endl;
    this->remove_client(client->fd);
    return -1;
  }

  // Skips the first 8 bytes (id and type) and the last 2 (null bytes)
  std::string packetBody(&buffer[8], &buffer[buffer.size() - 2]);
  int pid = u32_from_le({buffer[0], buffer[1], buffer[2], buffer[3]});
  int type = u32_from_le({buffer[4], buffer[5], buffer[6], buffer[7]});

  if (!client->authenticated) {
    if (packetBody == "password") {
      client->authenticated = true;
      std::cout << "[CLIENT] authenticated" << std::endl;
      auto authResponse = create_packet("", pid, SERVERDATA::AUTH_RESPONSE);
      auto responseValue = create_packet("", pid, SERVERDATA::RESPONSE_VALUE);
      client->send_packet(responseValue);
      client->send_packet(authResponse);
    } else {
      std::cout << "[CLIENT] authentication failed" << std::endl;
      auto authResponse = create_packet("", -1, SERVERDATA::AUTH_RESPONSE);
      auto responseValue = create_packet("", -1, SERVERDATA::RESPONSE_VALUE);
      client->send_packet(responseValue);
      client->send_packet(authResponse);
      this->remove_client(client->fd);
    }
  } else {
    Packet responsePacket{};
    if (type == SERVERDATA::EXECCOMAND) {
      auto response = run_command(packetBody);
      responsePacket = create_packet(response, pid, SERVERDATA::RESPONSE_VALUE);
      std::cout << "[CLIENT] command executed" << std::endl;
    } else {
      responsePacket = create_packet("", -1, SERVERDATA::RESPONSE_VALUE);
      std::cout << "[CLIENT] invalid packet type" << std::endl;
    }

    client->send_packet(responsePacket);
  }

  return 0;
}

void RconServer::add_client(int fd) {
  epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLIN | EPOLLONESHOT;
  {
    std::unique_lock lock(this->epollMtx);
    epoll_ctl(this->epollFd, EPOLL_CTL_ADD, fd, &ev);
  }
  auto client = std::make_shared<Client>(fd);
  this->clients[fd] = std::move(client);
}

void RconServer::remove_client(int fd) {
  {
    std::unique_lock lock(this->epollMtx);
    epoll_ctl(this->epollFd, EPOLL_CTL_DEL, fd, nullptr);
  }
  this->clients.erase(fd);
  shutdown(fd, SHUT_RDWR);
  close(fd);
  std::cout << "[SERVER] client " << fd << " disconnected" << std::endl;
}
