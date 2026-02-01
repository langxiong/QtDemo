#include "common/controller/ControllerRuntime.h"

#include "common/log/Log.h"

#include <Poco/Net/SocketAddress.h>

#include <chrono>
#include <thread>

namespace common::controller {

ControllerRuntime::ControllerRuntime(const common::config::Config& cfg,
                                     const common::sensor::SensorPipeline& sensor,
                                     common::status::StatusStore& status)
    : _sensor(sensor),
      _status(status),
      _procManager(
          _ipcClient,
          Poco::Net::SocketAddress(cfg.getString("ipc.host", "127.0.0.1"), cfg.getInt("ipc.port", 45678)),
          common::algo::AlgoProcessManager::Params{
              "algo_worker",
              std::chrono::milliseconds(cfg.getInt("ipc.heartbeat_timeout_ms", 500)),
              std::chrono::milliseconds(cfg.getInt("ipc.restart_backoff_ms", 500)),
              static_cast<std::uint32_t>(cfg.getInt("ipc.restart_max", 10))}),
      _heartbeat(_ipcClient,
                 common::heartbeat::HeartbeatMonitor::Params{
                     std::chrono::milliseconds(cfg.getInt("ipc.heartbeat_interval_ms", 200)),
                     std::chrono::milliseconds(cfg.getInt("ipc.heartbeat_timeout_ms", 500)),
                     static_cast<std::uint32_t>(cfg.getInt("ipc.heartbeat_miss_threshold", 3))}) {
  const int rateHz = cfg.getInt("sensor.rate_hz", 200);
  _controlLoop = std::make_unique<common::control::ControlLoop>(
      _sensor, _ipcClient, _actuator, _status,
      common::control::ControlLoop::Params{rateHz, std::chrono::milliseconds(1)});
}

ControllerRuntime::~ControllerRuntime() { stop(); }

void ControllerRuntime::start() {
  if (_running.exchange(true)) return;
  _thread = std::thread(&ControllerRuntime::run, this);
}

void ControllerRuntime::stop() {
  if (!_running.exchange(false)) return;
  if (_thread.joinable()) _thread.join();
  if (_controlLoop) _controlLoop->stop();
  _heartbeat.stop();
  _ipcClient.disconnect();
}

void ControllerRuntime::run() {
  common::log::Log::SetThreadName("controller");

  common::status::StatusSnapshot snap;
  snap.systemState = common::status::SystemState::Starting;
  _status.update(snap);

  _heartbeat.start();

  while (_running.load()) {
    const bool connected = _procManager.ensureConnected();

    if (!connected) {
      snap.systemState = common::status::SystemState::Degraded;
      snap.algoHealth = common::status::AlgoHealthState::Disconnected;
      snap.lastError = common::status::ErrorCode::IpcConnectFailed;
      snap.lastErrorMessage = "IPC connect/start failed";
      snap.heartbeatTimeouts = _heartbeat.timeouts();
      snap.heartbeatRttMs = _heartbeat.lastRttMs();
      _status.update(snap);
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      continue;
    }

    if (_controlLoop) _controlLoop->start();

    const bool hbOk = _heartbeat.healthy();

    if (!hbOk) {
      snap.systemState = common::status::SystemState::Degraded;
      snap.algoHealth = common::status::AlgoHealthState::Unhealthy;
      snap.lastError = common::status::ErrorCode::HeartbeatTimeout;
      snap.lastErrorMessage = "Heartbeat unhealthy";
      snap.heartbeatTimeouts = _heartbeat.timeouts();
      snap.heartbeatRttMs = _heartbeat.lastRttMs();

      (void)_procManager.restart();
      snap.algoRestarts = _procManager.restartCount();
      _status.update(snap);

      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      continue;
    }

    snap.systemState = common::status::SystemState::Running;
    snap.algoHealth = common::status::AlgoHealthState::Healthy;
    snap.heartbeatRttMs = _heartbeat.lastRttMs();
    snap.heartbeatTimeouts = _heartbeat.timeouts();
    snap.algoRestarts = _procManager.restartCount();
    _status.update(snap);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  snap.systemState = common::status::SystemState::Stopping;
  _status.update(snap);
}

} // namespace common::controller
