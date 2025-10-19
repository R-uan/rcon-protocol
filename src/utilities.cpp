#include "utilities.hpp"
#include <cstring>

Packet create_packet(const std::string_view data, int32_t id, int32_t type) {
  const int32_t dataSize = static_cast<int32_t>(data.size() + 10);

  std::vector<char> tempData(dataSize + 4);

  std::memcpy(tempData.data() + 0, &dataSize, sizeof(dataSize));
  std::memcpy(tempData.data() + 4, &id, sizeof(id));
  std::memcpy(tempData.data() + 8, &type, sizeof(type));
  std::memcpy(tempData.data() + 12, data.data(), data.size());
  tempData.push_back('\x00');
  tempData.push_back('\x00');

  Packet packet;
  packet.id = id;
  packet.size = dataSize;
  packet.type = type;
  packet.data = tempData;

  return packet;
}

void Logger::stop() {
  if (this->running.exchange(false)) {
    this->cv.notify_all();
    if (this->thread.joinable())
      this->thread.join();
  }
}

void Logger::log(std::string msg) {
  {
    std::lock_guard lock(this->queueMtx);
    this->logQueue.push(msg);
  }
  this->cv.notify_one();
}

void Logger::worker() {
  std::unique_lock lock(this->queueMtx);
  while (this->running.load()) {
    this->cv.wait(lock, [this]() {
      return !this->running.load() || !this->logQueue.empty();
    });

    if (!this->running.load() && this->logQueue.empty())
      return;

    std::string msg = std::move(this->logQueue.front());
    this->logQueue.pop();

    lock.unlock();
    std::cout << msg << '\n';
    lock.lock();
  }
}
