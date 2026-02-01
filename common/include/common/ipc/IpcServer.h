#pragma once

#include "common/ipc/Protocol.h"

#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/StreamSocket.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>

namespace common::ipc {

class IpcServer {
public:
  explicit IpcServer(const Poco::Net::SocketAddress& bindAddr);

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
  bool sendFrame(MsgType type, const void* payload, std::size_t payloadSize, std::chrono::milliseconds timeout);
  bool receiveFrame(FrameHeader& header, std::string& payload, std::chrono::milliseconds timeout);

  Poco::Net::SocketAddress _bind;
  Poco::Net::ServerSocket _srv;

  mutable std::mutex _mu;
  Poco::Net::StreamSocket _client;
  bool _connected{false};
};

} // namespace common::ipc
