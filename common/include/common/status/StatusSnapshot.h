#pragma once

#include "common/status/Models.h"

#include <cstdint>
#include <mutex>
#include <string>

namespace common::status {

struct StatusSnapshot {
  SystemState systemState{SystemState::Starting};
  SafetyState safetyState{SafetyState::Normal};
  AlgoHealthState algoHealth{AlgoHealthState::Unknown};

  ErrorCode lastError{ErrorCode::Ok};
  std::string lastErrorMessage;

  // Metrics (demo-oriented)
  double sensorRateHz{0.0};
  std::uint64_t sensorSeq{0};
  std::uint64_t sensorMissedDeadlines{0};

  double controlLoopHz{0.0};
  double algoLatencyMs{0.0};

  // Latest control/actuator
  double lastCommand{0.0};
  double actuatorPosition{0.0};
  double actuatorVelocity{0.0};

  double heartbeatRttMs{0.0};
  std::uint64_t heartbeatTimeouts{0};
  std::uint64_t algoRestarts{0};

  std::string estopReason;
};

class StatusStore {
public:
  void update(const StatusSnapshot& snapshot) {
    std::lock_guard<std::mutex> lk(_mu);
    _snapshot = snapshot;
  }

  StatusSnapshot read() const {
    std::lock_guard<std::mutex> lk(_mu);
    return _snapshot;
  }

private:
  mutable std::mutex _mu;
  StatusSnapshot _snapshot;
};

} // namespace common::status
