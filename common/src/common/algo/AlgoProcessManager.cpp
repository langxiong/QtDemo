#include "common/algo/AlgoProcessManager.h"

#include "common/ipc/Protocol.h"
#include "common/log/Log.h"

#include <Poco/Exception.h>
#include <Poco/Pipe.h>
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
    Poco::Process::Args args;

    Poco::Pipe outPipe;
    common::log::Info("algo", "starting algo_worker: " + _params.executable);
    _processHandle.emplace(Poco::Process::launch(_params.executable, args, nullptr, &outPipe, nullptr));
    outPipe.close(Poco::Pipe::CLOSE_WRITE);

    common::log::Info("algo", "waiting for algo_worker ready byte (timeout " + std::to_string(_params.readyTimeout.count()) + " ms)...");
    std::mutex mu;
    std::condition_variable cv;
    std::atomic<bool> gotByte{false};
    std::atomic<int> byteValue{-1};
    std::thread reader([&outPipe, &gotByte, &byteValue, &cv]() {
      unsigned char b = 0;
      const int n = outPipe.readBytes(&b, 1);
      if (n == 1) {
        byteValue.store(static_cast<unsigned char>(b));
        gotByte.store(true);
      }
      cv.notify_one();
    });

    std::unique_lock<std::mutex> lock(mu);
    const bool ready = cv.wait_for(lock, _params.readyTimeout, [&gotByte]() { return gotByte.load(); });

    if (ready && gotByte.load() && byteValue.load() == static_cast<int>(common::ipc::kReadyByte)) {
      lock.unlock();
      reader.join();
      outPipe.close(Poco::Pipe::CLOSE_READ);
      common::log::Info("algo", "algo_worker ready byte received");
      return true;
    }
    reader.detach();
    try { outPipe.close(Poco::Pipe::CLOSE_BOTH); } catch (...) {}
    common::log::Error("algo", "timeout or invalid byte waiting for algo_worker ready signal");
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
