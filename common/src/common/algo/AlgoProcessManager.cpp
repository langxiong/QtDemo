#include "common/algo/AlgoProcessManager.h"

#include "common/log/Log.h"
#include "common/process/ProcessLauncher.h"

#include <thread>

namespace common::algo {

AlgoProcessManager::AlgoProcessManager(common::ipc::IpcClient& client, Poco::Net::SocketAddress addr, Params params)
    : _client(client), _addr(std::move(addr)), _params(std::move(params)) {}

bool AlgoProcessManager::ensureConnected() {
  if (_client.isConnected()) return true;

  // Try connect first (maybe worker already running)
  if (connectIpc()) return true;

  // Start then connect
  if (!startProcess()) return false;
  return connectIpc();
}

bool AlgoProcessManager::restart() {
  if (_restartCount >= _params.maxRestarts) {
    common::log::Log::Error("algo", "restart limit reached");
    return false;
  }

  _client.disconnect();

  std::this_thread::sleep_for(_params.restartBackoff);

  if (!startProcess()) return false;
  if (!connectIpc()) return false;

  ++_restartCount;
  common::log::Log::Warn("algo", "algo_worker restarted, count=" + std::to_string(_restartCount));
  return true;
}

bool AlgoProcessManager::startProcess() {
  common::log::Log::Info("algo", "starting algo_worker: " + _params.executable);
  if (!common::process::ProcessLauncher::startDetached(_params.executable, "")) {
    common::log::Log::Error("algo", "failed to start algo_worker");
    return false;
  }
  return true;
}

bool AlgoProcessManager::connectIpc() {
  common::log::Log::Info("algo", "connecting to algo_worker at " + _addr.toString());
  const bool ok = _client.connect(_addr, _params.connectTimeout);
  if (!ok) {
    common::log::Log::Warn("algo", "connect failed");
  }
  return ok;
}

} // namespace common::algo
