#pragma once

#include "common/ipc/IpcClient.h"
#include "common/ipc/Protocol.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

namespace common::heartbeat {

class HeartbeatMonitor {
public:
  struct Params {
    std::chrono::milliseconds interval{200};
    std::chrono::milliseconds timeout{500};
    std::uint32_t missThreshold{3};
  };

  HeartbeatMonitor(common::ipc::IpcClient& client, Params params);
  ~HeartbeatMonitor();

  void start();
  void stop();

  bool healthy() const { return _healthy.load(); }
  double lastRttMs() const { return _lastRttMs.load(); }
  std::uint64_t timeouts() const { return _timeouts.load(); }

private:
  void run();

  common::ipc::IpcClient& _client;
  Params _params;

  std::atomic<bool> _running{false};
  std::atomic<bool> _healthy{false};
  std::atomic<double> _lastRttMs{0.0};
  std::atomic<std::uint64_t> _timeouts{0};

  std::thread _thread;
};

} // namespace common::heartbeat
