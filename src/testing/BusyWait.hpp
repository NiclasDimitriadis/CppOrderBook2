#pragma once
// wait for n nanoseconds without context switch

#include <chrono>
#include <cstdint>

namespace BusyWait {
void busy_wait(std::int64_t n) {
  auto start_time = std::chrono::high_resolution_clock::now();
  while (duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now() - start_time)
             .count() < n) {
  };
}
} // namespace BusyWait
