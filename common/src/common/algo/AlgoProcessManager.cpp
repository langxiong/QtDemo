#include "common/algo/AlgoProcessManager.h"

#include "common/log/Log.h"

#include <Poco/Exception.h>
#include <Poco/NamedEvent.h>
#include <Poco/Process.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace common::algo {

AlgoProcessManager::AlgoProcessManager(common::ipc::IpcClient& client, Poco::Net::SocketAddress addr, Params params)
    : _client(client), _addr(std::move(addr)), _params(std::move(params)) {}

AlgoProcessManager::~AlgoProcessManager() {
    cleanupProcess();
}

bool AlgoProcessManager::ensureConnected() {
  if (_client.isConnected()) return true;

  if (connectIpc()) return true;

  if (!startProcess()) return false;
  return connectIpc();
}

bool AlgoProcessManager::restart() {
  common::log::Info("algo", "restart() called (current restartCount=" + std::to_string(_restartCount) +
                     " max=" + std::to_string(_params.maxRestarts) + ")");
  if (_restartCount >= _params.maxRestarts) {
    common::log::Error("algo", "restart limit reached");
    return false;
  }

  _client.disconnect();
  cleanupProcess();

  std::this_thread::sleep_for(_params.restartBackoff);

  if (!startProcess()) return false;
  if (!connectIpc()) return false;

  ++_restartCount;
  common::log::Warn("algo", "algo_worker restarted, count=" + std::to_string(_restartCount));
  return true;
}

void AlgoProcessManager::cleanupProcess() {
    if (_processHandle) {
        try {
            Poco::Process::kill(*_processHandle);
            _processHandle->wait();
        } catch (const Poco::Exception&) {
            // Ignore if process is already gone
        }
        _processHandle.reset();
    }
}

bool AlgoProcessManager::startProcess() {
  try {
    const std::string readyEventName = "MedicalRobotAlgoReady_" + std::to_string(Poco::Process::id());
    Poco::NamedEvent readyEvent(readyEventName);

    Poco::Process::Args args;
    args.push_back("--ready-event=" + readyEventName);

    common::log::Info("algo", "starting algo_worker: " + _params.executable);
    _processHandle.emplace(Poco::Process::launch(_params.executable, args, nullptr, nullptr, nullptr));

    common::log::Info("algo", "waiting for algo_worker to be ready (named event)...");
    std::mutex mu;
    std::condition_variable cv;
    std::atomic<bool> signaled{false};
    // NamedEvent has no timed wait; waiter thread blocks on wait(), caller blocks on cv with timeout.
    std::thread waiter([&readyEvent, &signaled, &cv]() {
      readyEvent.wait();
      signaled.store(true);
      cv.notify_one();
    });

    constexpr std::chrono::seconds readyTimeout{10};
    std::unique_lock<std::mutex> lock(mu);
    const bool got = cv.wait_for(lock, readyTimeout, [&signaled]() { return signaled.load(); });

    if (got) {
      lock.unlock();
      waiter.join();
      common::log::Info("algo", "algo_worker signaled ready");
      return true;
    }
    waiter.detach();
    common::log::Error("algo", "timeout waiting for algo_worker ready signal");
    cleanupProcess();
    return false;
  } catch (const Poco::Exception& e) {
    common::log::Error("algo", "failed to launch algo_worker: " + e.displayText());
    return false;
  }
}

bool AlgoProcessManager::connectIpc() {
  common::log::Info("algo", "connecting to algo_worker at " + _addr.toString());
  const bool ok = _client.connect(_addr, _params.connectTimeout);
  if (!ok) {
    common::log::Warn("algo", "connect failed");
  }
  return ok;
}

} // namespace common::algo
