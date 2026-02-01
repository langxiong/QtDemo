#pragma once

#include <cstdint>
#include <string>

namespace common::status {

enum class SystemState : std::uint8_t {
  Starting = 0,
  Running,
  Degraded,
  Stopping,
  Stopped
};

enum class SafetyState : std::uint8_t {
  Normal = 0,
  EStopLatched
};

enum class AlgoHealthState : std::uint8_t {
  Unknown = 0,
  Healthy,
  Unhealthy,
  Disconnected
};

enum class ErrorCode : std::uint32_t {
  Ok = 0,
  ConfigLoadFailed = 1000,
  IpcConnectFailed = 2000,
  HeartbeatTimeout = 2100,
  AlgoCrashed = 2200,
  SafetyLimitExceeded = 3000,
  ManualEStop = 3100
};

inline const char* ToString(SystemState s) {
  switch (s) {
    case SystemState::Starting:
      return "Starting";
    case SystemState::Running:
      return "Running";
    case SystemState::Degraded:
      return "Degraded";
    case SystemState::Stopping:
      return "Stopping";
    case SystemState::Stopped:
      return "Stopped";
  }
  return "Unknown";
}

inline const char* ToString(SafetyState s) {
  switch (s) {
    case SafetyState::Normal:
      return "Normal";
    case SafetyState::EStopLatched:
      return "EStopLatched";
  }
  return "Unknown";
}

inline const char* ToString(AlgoHealthState s) {
  switch (s) {
    case AlgoHealthState::Unknown:
      return "Unknown";
    case AlgoHealthState::Healthy:
      return "Healthy";
    case AlgoHealthState::Unhealthy:
      return "Unhealthy";
    case AlgoHealthState::Disconnected:
      return "Disconnected";
  }
  return "Unknown";
}

} // namespace common::status
