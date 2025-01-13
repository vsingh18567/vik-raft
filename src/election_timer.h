#pragma once
#include <chrono>
#include <random>

namespace vikraft {

class ElectionTimer {
public:
  ElectionTimer() { reset(); }

  void reset() {
    std::random_device rd;
    rand_gen_ = std::mt19937(rd());
    std::uniform_int_distribution<> dist(1000, 2000);
    timeout_ = std::chrono::steady_clock::now() +
               std::chrono::milliseconds(dist(rand_gen_));
  }

  bool has_elapsed() const {
    return std::chrono::steady_clock::now() >= timeout_;
  }

private:
  std::chrono::steady_clock::time_point timeout_;
  std::mt19937 rand_gen_;
};

} // namespace vikraft