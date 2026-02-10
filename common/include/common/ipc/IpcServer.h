#pragma once

#include "common/ipc/Protocol.h"

#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/StreamSocket.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

namespace common::ipc {

class IpcServer {
public:
  explicit IpcServer(const Poco::Net::SocketAddress& bindAddr);
  ~IpcServer();

  bool start();
  void stop();

  // Blocks until a client connects or timeout.
  bool acceptOne(std::chrono::milliseconds timeout);

  bool isConnected() const;

  bool tryReceivePing(Ping& ping, std::chrono::milliseconds timeout);
  bool tryReceiveSensorFrame(SensorFrame& frame, std::chrono::milliseconds timeout);

  bool sendPong(const Pong& pong, std::chrono::milliseconds timeout);
  bool sendAlgoResult(const AlgoResult& result, std::chrono::milliseconds timeout);

private:
  void receiverLoop();
  bool receiveOneFrameLocked(FrameHeader& header, std::string& payload);

  bool sendFrame(MsgType type, const void* payload, std::size_t payloadSize, std::chrono::milliseconds timeout);

  Poco::Net::SocketAddress _bind;
  Poco::Net::ServerSocket _srv;

  mutable std::mutex _mu;
  Poco::Net::StreamSocket _client;
  std::atomic<bool> _connected{false};

  std::thread _receiverThread;
  std::atomic<bool> _receiverRunning{false};

  std::mutex _queueMu;
  std::queue<Ping> _pingQueue;
  std::queue<SensorFrame> _sensorFrameQueue;
  std::condition_variable _pingCv;
  std::condition_variable _sensorFrameCv;
};

} // namespace common::ipc
