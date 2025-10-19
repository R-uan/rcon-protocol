#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <queue>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

enum SERVERDATA {
  AUTH = 3,
  AUTH_RESPONSE = 2,
  EXECCOMAND = 2,
  RESPONSE_VALUE = 0
};

struct Packet {
  int size{-1};
  int id{-1};
  int type{-1};
  std::vector<char> data{};
};

Packet create_packet(const std::string_view data, int32_t id, int32_t type);

inline int u32_from_le(const std::vector<uint8_t> bytes) {
  return static_cast<int>(bytes[0] | bytes[1] << 8 | bytes[2] << 16 |
                          bytes[3] << 24);
}

class Logger {
public:
  inline Logger() : running(true) {
    this->thread = std::thread(&Logger::worker, this);
  };

  inline ~Logger() { this->stop(); }

  void stop();
  void log(std::string msg);

private:
  void worker();

  std::thread thread;
  std::mutex queueMtx;
  std::atomic_bool running;
  std::condition_variable cv;
  std::queue<std::string> logQueue;
};
