#include "common/controller/ControllerRuntime.h"

#include "common/log/Log.h"

#include <Poco/Net/SocketAddress.h>
#include <Poco/Path.h>

#include <chrono>
#include <thread>

namespace common::controller {


ControllerRuntime::ControllerRuntime(const common::config::Config& cfg,
                                     const common::sensor::SensorPipeline& sensor,
                                     common::status::StatusStore& status,
                                     const std::string& applicationDirPath)
    : _sensor(sensor),
      _status(status),
      _procManager(
          _ipcClient,
          Poco::Net::SocketAddress(cfg.getString("ipc.host", "127.0.0.1"), static_cast<Poco::UInt16>(cfg.getInt("ipc.port", 45678))),
          common::algo::AlgoProcessManager::Params{
              Poco::Path(applicationDirPath).append("algo_worker.exe").toString(),
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
  common::log::SetThreadName("controller");

  common::status::StatusSnapshot snap;
  snap.systemState = common::status::SystemState::Starting;
  _status.update(snap);

  _heartbeat.start();

  constexpr std::chrono::milliseconds kConnectionGraceMs{2000};
  constexpr std::uint32_t kRestartAfterConsecutiveUnhealthy{3};

  while (_running.load()) {
    const bool connected = _procManager.ensureConnected();

    if (!connected) {
      _wasConnected = false;
      _connectionGraceEnd.reset();
      _hasBeenHealthy = false;
      _controlLoopStarted = false;
      _consecutiveUnhealthy = 0;
      common::log::Debug("controller", "not connected; ensureConnected failed");
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

    if (!_wasConnected) {
      _connectionGraceEnd = std::chrono::steady_clock::now() + kConnectionGraceMs;
      _wasConnected = true;
      common::log::Info("controller", "connected; grace period started (2s)");
    }
    const bool hbOk = _heartbeat.healthy();
    if (hbOk) {
      _hasBeenHealthy = true;
      if (!_controlLoopStarted && _controlLoop) {
        _controlLoop->start();
        _controlLoopStarted = true;
        common::log::Info("controller", "heartbeat established; control loop started");
      }
      if (_consecutiveUnhealthy > 0) {
        common::log::Debug("controller", "heartbeat OK again; consecutiveUnhealthy reset from " + std::to_string(_consecutiveUnhealthy));
      }
      _consecutiveUnhealthy = 0;
    }
    const bool inGrace = _connectionGraceEnd.has_value() &&
                         std::chrono::steady_clock::now() < *_connectionGraceEnd;

    common::log::Debug("controller", "hbOk=" + std::string(hbOk ? "true" : "false") +
                         " inGrace=" + std::string(inGrace ? "true" : "false") +
                         " hasBeenHealthy=" + std::string(_hasBeenHealthy ? "true" : "false") +
                         " consecutiveUnhealthy=" + std::to_string(_consecutiveUnhealthy));

    if (!hbOk && !inGrace && _hasBeenHealthy) {
      ++_consecutiveUnhealthy;
      common::log::Info("controller", "heartbeat unhealthy: consecutiveUnhealthy=" + std::to_string(_consecutiveUnhealthy) +
                         "/" + std::to_string(kRestartAfterConsecutiveUnhealthy) +
                         " timeouts=" + std::to_string(_heartbeat.timeouts()));
      snap.systemState = common::status::SystemState::Degraded;
      snap.algoHealth = common::status::AlgoHealthState::Unhealthy;
      snap.lastError = common::status::ErrorCode::HeartbeatTimeout;
      snap.lastErrorMessage = "Heartbeat unhealthy";
      snap.heartbeatTimeouts = _heartbeat.timeouts();
      snap.heartbeatRttMs = _heartbeat.lastRttMs();
      snap.algoRestarts = _procManager.restartCount();
      _status.update(snap);

      if (_consecutiveUnhealthy >= kRestartAfterConsecutiveUnhealthy) {
        common::log::Warn("controller", "restarting algo_worker: consecutiveUnhealthy=" + std::to_string(_consecutiveUnhealthy) +
                           " (threshold " + std::to_string(kRestartAfterConsecutiveUnhealthy) + ")");
        (void)_procManager.restart();
        snap.algoRestarts = _procManager.restartCount();
        _status.update(snap);
        _wasConnected = false;
        _connectionGraceEnd.reset();
        _hasBeenHealthy = false;
        _controlLoopStarted = false;
        _consecutiveUnhealthy = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      continue;
    }
    if (!hbOk && (inGrace || !_hasBeenHealthy)) {
      snap.systemState = common::status::SystemState::Running;
      snap.algoHealth = common::status::AlgoHealthState::Healthy;
      snap.heartbeatRttMs = _heartbeat.lastRttMs();
      snap.heartbeatTimeouts = _heartbeat.timeouts();
      snap.algoRestarts = _procManager.restartCount();
      _status.update(snap);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
