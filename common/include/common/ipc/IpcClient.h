#pragma once

#include "common/ipc/Protocol.h"

#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/StreamSocket.h>

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>

namespace common::ipc {

class IpcClient {
public:
  IpcClient() = default;

  bool connect(const Poco::Net::SocketAddress& addr, std::chrono::milliseconds timeout);
  void disconnect();
  bool isConnected() const;

  bool sendPing(const Ping& ping, std::chrono::milliseconds timeout);
  bool sendSensorFrame(const SensorFrame& frame, std::chrono::milliseconds timeout);

  bool tryReceivePong(Pong& pong, std::chrono::milliseconds timeout);
  bool tryReceiveAlgoResult(AlgoResult& result, std::chrono::milliseconds timeout);

private:
  bool sendFrame(MsgType type, const void* payload, std::size_t payloadSize, std::chrono::milliseconds timeout);
  bool receiveFrame(FrameHeader& header, std::string& payload, std::chrono::milliseconds timeout);

  mutable std::mutex _mu;
  Poco::Net::StreamSocket _sock;
  bool _connected{false};
};

} // namespace common::ipc
