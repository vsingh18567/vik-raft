#include "timer.h"

ElectionTimer::ElectionTimer() : engine_(std::random_device{}()) { Reset(); }

void ElectionTimer::Reset() {
  std::uniform_int_distribution<> dist(500, 1000);
  timeout_ = std::chrono::milliseconds(dist(engine_));
  start_ = std::chrono::steady_clock::now();
}

bool ElectionTimer::HasExpired() const {
  auto now = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);
  return elapsed >= timeout_;
}
