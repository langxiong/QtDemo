#pragma once

#include "common/config/Config.h"
#include "common/control/ActuatorSimulator.h"
#include "common/sensor/SensorPipeline.h"
#include "common/status/StatusSnapshot.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

struct JointState;
struct ControlCommand;

namespace dds_core {
class DDSNode;
}

namespace controller_app {

class ControllerRuntimeDds {
public:
  ControllerRuntimeDds(const common::config::Config& cfg,
                       const common::sensor::SensorPipeline& sensor,
                       common::status::StatusStore& status,
                       const std::string& applicationDirPath);
  ~ControllerRuntimeDds();

  void start();
  void stop();

private:
  void runPublisher();
  void runSubscriber();
  void runStatusUpdater();

  const common::sensor::SensorPipeline& _sensor;
  common::status::StatusStore& _status;
  std::string _algoExePath;

  std::unique_ptr<dds_core::DDSNode> _node;
  void* _jsWriter{nullptr};
  void* _ccReader{nullptr};

  common::control::ActuatorSimulator _actuator;
  std::atomic<double> _lastCmdValue{0.0};
  std::atomic<uint64_t> _lastCmdTimestamp{0};
  std::atomic<bool> _hasAlgo{false};

  std::atomic<bool> _running{false};
  std::thread _pubThread;
  std::thread _subThread;
  std::thread _statusThread;

  int _domainId{0};
  int _sensorRateHz{200};
  std::chrono::milliseconds _algoTimeoutMs{500};
};

}  // namespace controller_app
