#pragma once

#include "common/ipc/Protocol.h"

#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/StreamSocket.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

namespace common::ipc {

class IpcClient {
public:
  IpcClient() = default;
  ~IpcClient();

  bool connect(const Poco::Net::SocketAddress& addr, std::chrono::milliseconds timeout);
  void disconnect();
  bool isConnected() const;

  bool sendPing(const Ping& ping, std::chrono::milliseconds timeout);
  bool sendSensorFrame(const SensorFrame& frame, std::chrono::milliseconds timeout);

  bool tryReceivePong(Pong& pong, std::chrono::milliseconds timeout);
  bool tryReceiveAlgoResult(AlgoResult& result, std::chrono::milliseconds timeout);

private:
  void receiverLoop();
  bool receiveOneFrameLocked(FrameHeader& header, std::string& payload);

  bool sendFrame(MsgType type, const void* payload, std::size_t payloadSize, std::chrono::milliseconds timeout);

  mutable std::mutex _mu;
  Poco::Net::StreamSocket _sock;
  std::atomic<bool> _connected{false};

  std::thread _receiverThread;
  std::atomic<bool> _receiverRunning{false};

  std::mutex _queueMu;
  std::queue<Pong> _pongQueue;
  std::queue<AlgoResult> _algoResultQueue;
  std::condition_variable _pongCv;
  std::condition_variable _algoResultCv;
};

} // namespace common::ipc
