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
#include <memory>
#include <thread>

namespace common::controller {

class ControllerRuntime {
public:
  ControllerRuntime(const common::config::Config& cfg,
                    const common::sensor::SensorPipeline& sensor,
                    common::status::StatusStore& status);
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
};

} // namespace common::controller
