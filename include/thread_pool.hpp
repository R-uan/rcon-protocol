#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
public:
  inline ThreadPool(size_t n) : stop(false) {
    for (size_t i = 0; i < n; i++) {
      this->workers.emplace_back([this]() {
        while (true) {
          std::function<void()> task;
          {
            std::unique_lock lock(this->mtx);
            this->cv.wait(
                lock, [this]() { return this->stop || !this->tasks.empty(); });
            if (this->stop && this->tasks.empty())
              return;
            task = std::move(this->tasks.front());
            this->tasks.pop();
          }
          task();
        }
      });
    }
  }

  template <typename F> inline void enqueue(F &&f) {
    std::cout << "[THREAD POOL] new task enqueued" << std::endl;
    {
      std::unique_lock lock(this->mtx);
      this->tasks.emplace(std::forward<F>(f));
    }

    this->cv.notify_one();
  }

  inline ~ThreadPool() {
    {
      std::unique_lock lock(this->mtx);
      stop = true;
    }
    this->cv.notify_all();
  }

private:
  bool stop;
  std::mutex mtx;
  std::condition_variable cv;
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
};
