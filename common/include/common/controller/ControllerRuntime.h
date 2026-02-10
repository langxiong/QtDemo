#pragma once

#include "common/algo/AlgoProcessManager.h"
#include "common/config/Config.h"
#include "common/control/ActuatorSimulator.h"
#include "common/control/ControlLoop.h"
#include "common/heartbeat/HeartbeatMonitor.h"
#include "common/ipc/IpcClient.h"
#include "common/sensor/SensorPipeline.h"
#include "common/status/StatusSnapshot.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>

namespace common::controller {

class ControllerRuntime {
public:
  ControllerRuntime(const common::config::Config& cfg,
                    const common::sensor::SensorPipeline& sensor,
                    common::status::StatusStore& status,
                    const std::string& applicationDirPath);
  ~ControllerRuntime();

  void start();
  void stop();

private:
  void run();

  const common::sensor::SensorPipeline& _sensor;
  common::status::StatusStore& _status;

  common::ipc::IpcClient _ipcClient;
  common::algo::AlgoProcessManager _procManager;
  common::heartbeat::HeartbeatMonitor _heartbeat;

  common::control::ActuatorSimulator _actuator;
  std::unique_ptr<common::control::ControlLoop> _controlLoop;

  std::atomic<bool> _running{false};
  std::thread _thread;

  bool _wasConnected{false};
  std::optional<std::chrono::steady_clock::time_point> _connectionGraceEnd;
  bool _hasBeenHealthy{false};
  /** Control loop started only after first Pong, so we don't flood with SensorFrames before heartbeat. */
  bool _controlLoopStarted{false};
  /** Consecutive run() iterations with !healthy; restart only when this reaches kRestartAfterConsecutiveUnhealthy. */
  std::uint32_t _consecutiveUnhealthy{0};
};

} // namespace common::controller
