#pragma once

#include <mutex>

struct Client {
  bool authenticated;
  std::mutex mtx;
};
