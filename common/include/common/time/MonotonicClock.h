#pragma once

#include <chrono>
#include <cstdint>

namespace common::time {

inline std::uint64_t NowMonotonicNs() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

inline double NsToMs(std::uint64_t ns) { return static_cast<double>(ns) / 1e6; }

} // namespace common::time
