#pragma once

#include "common/control/ActuatorSimulator.h"
#include "common/control/ControlModels.h"
#include "common/ipc/IpcClient.h"
#include "common/ipc/Protocol.h"
#include "common/sensor/SensorPipeline.h"
#include "common/status/StatusSnapshot.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace common::control {

class ControlLoop {
public:
  struct Params {
    int rateHz{200};
    std::chrono::milliseconds algoReadTimeout{1};
  };

  ControlLoop(const common::sensor::SensorPipeline& sensor,
              common::ipc::IpcClient& ipc,
              ActuatorSimulator& actuator,
              common::status::StatusStore& status,
              Params params);
  ~ControlLoop();

  void start();
  void stop();

  ControlCommand lastCommand() const;

private:
  void run();
  ControlCommand computeCommand(const common::sensor::SensorSnapshot& s, const common::ipc::AlgoResult* algo);

  const common::sensor::SensorPipeline& _sensor;
  common::ipc::IpcClient& _ipc;
  ActuatorSimulator& _actuator;
  common::status::StatusStore& _status;
  Params _params;

  std::atomic<bool> _running{false};
  std::thread _thread;

  mutable std::mutex _mu;
  ControlCommand _lastCmd;

  std::atomic<bool> _hasAlgo{false};
  common::ipc::AlgoResult _lastAlgo;
};

} // namespace common::control
