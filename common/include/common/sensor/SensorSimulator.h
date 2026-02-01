#pragma once

#include "common/sensor/SensorModels.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <random>

namespace common::sensor {

class SensorSimulator {
public:
  struct Params {
    int rateHz{200};
  };

  explicit SensorSimulator(Params params);

  SensorSample generate();

  double effectiveRateHz() const { return _effectiveRateHz.load(); }
  std::uint64_t missedDeadlines() const { return _missedDeadlines.load(); }

private:
  Params _params;
  std::uint64_t _seq{0};
  std::mt19937 _rng;
  std::normal_distribution<double> _noise{0.0, 0.02};

  std::chrono::steady_clock::time_point _last;
  std::atomic<double> _effectiveRateHz{0.0};
  std::atomic<std::uint64_t> _missedDeadlines{0};
};

} // namespace common::sensor
