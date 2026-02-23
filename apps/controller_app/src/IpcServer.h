#pragma once

#include "common/status/StatusSnapshot.h"
#include "common/sensor/SensorPipeline.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

class DeviceSimulator;

namespace common::config {
struct Config;
}

namespace controller_app {

class IpcServer {
public:
  IpcServer(const common::config::Config& cfg, DeviceSimulator& device,
            common::status::StatusStore& statusStore,
            const common::sensor::SensorPipeline& sensor);
  ~IpcServer();

  void start();
  void stop();

private:
  void run();
  std::string handleRequest(const std::string& requestJson);

  const common::config::Config& _cfg;
  DeviceSimulator& _device;
  common::status::StatusStore& _statusStore;
  const common::sensor::SensorPipeline& _sensor;

  std::atomic<bool> _running{false};
  std::thread _thread;
  int _listenPort{9123};
};

}  // namespace controller_app
