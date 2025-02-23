#pragma once

#include <chrono>
#include <random>

class ElectionTimer {
public:
  ElectionTimer();
  void Reset();

  bool HasExpired() const;

private:
  std::mt19937 engine_;
  std::chrono::steady_clock::time_point start_;
  std::chrono::milliseconds timeout_;
};
