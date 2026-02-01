#pragma once

#include <cstdint>

namespace common::sensor {

struct SensorSample {
  std::uint64_t seq{0};
  std::uint64_t monotonicNs{0};

  double valueA{0.0};
  double valueB{0.0};
  double valueC{0.0};
};

struct SensorSnapshot {
  SensorSample latest;

  double effectiveRateHz{0.0};
  std::uint64_t missedDeadlines{0};
};

} // namespace common::sensor
