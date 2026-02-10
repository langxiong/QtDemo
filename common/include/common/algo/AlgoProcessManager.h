#pragma once

#include "common/ipc/IpcClient.h"

#include <Poco/Net/SocketAddress.h>
#include <Poco/Process.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <optional>

namespace common::algo {

class AlgoProcessManager {
public:
  struct Params {
    std::string executable{"algo_worker"};
    std::chrono::milliseconds connectTimeout{500};
    std::chrono::milliseconds restartBackoff{500};
    std::uint32_t maxRestarts{10};
  };

  AlgoProcessManager(common::ipc::IpcClient& client, Poco::Net::SocketAddress addr, Params params);
  ~AlgoProcessManager();

  // Ensure algo process is running and IPC is connected.
  bool ensureConnected();

  // Called when algo is deemed unhealthy/unavailable.
  bool restart();

  std::uint64_t restartCount() const { return _restartCount; }

private:
  void cleanupProcess();
  bool startProcess();
  bool connectIpc();

  common::ipc::IpcClient& _client;
  Poco::Net::SocketAddress _addr;
  Params _params;

  std::optional<Poco::ProcessHandle> _processHandle;
  std::uint64_t _restartCount{0};
};

} // namespace common::algo
