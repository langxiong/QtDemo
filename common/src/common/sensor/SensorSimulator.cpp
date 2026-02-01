#include "common/sensor/SensorSimulator.h"

#include <cmath>

namespace common::sensor {

static std::uint64_t NowMonotonicNs() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

SensorSimulator::SensorSimulator(Params params)
    : _params(params), _rng(std::random_device{}()), _last(std::chrono::steady_clock::now()) {}

SensorSample SensorSimulator::generate() {
  using clock = std::chrono::steady_clock;

  const auto now = clock::now();
  const auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(now - _last).count();
  _last = now;

  if (dt > 0.0) {
    _effectiveRateHz.store(1.0 / dt);
  }

  const double t = static_cast<double>(_seq) / static_cast<double>(std::max(1, _params.rateHz));

  SensorSample s;
  s.seq = ++_seq;
  s.monotonicNs = NowMonotonicNs();

  s.valueA = std::sin(2.0 * 3.1415926535 * 0.8 * t) + _noise(_rng);
  s.valueB = std::cos(2.0 * 3.1415926535 * 0.3 * t) + _noise(_rng);
  s.valueC = 0.5 * std::sin(2.0 * 3.1415926535 * 0.1 * t) + _noise(_rng);

  // Missed-deadline heuristic: if loop period exceeds 2x expected.
  const double expected = 1.0 / static_cast<double>(std::max(1, _params.rateHz));
  if (dt > 2.0 * expected) {
    _missedDeadlines.fetch_add(1);
  }

  return s;
}

} // namespace common::sensor
