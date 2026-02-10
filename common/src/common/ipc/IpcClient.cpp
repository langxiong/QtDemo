#include "common/ipc/IpcClient.h"

#include "common/ipc/BinaryCodec.h"
#include "common/log/Log.h"

#include <Poco/Exception.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/SocketStream.h>

#include <sstream>

namespace common::ipc {

static void ApplyTimeout(Poco::Net::StreamSocket& sock, std::chrono::milliseconds timeout) {
  Poco::Timespan ts(static_cast<long>(timeout.count() / 1000),
                    static_cast<long>((timeout.count() % 1000) * 1000));
  sock.setReceiveTimeout(ts);
  sock.setSendTimeout(ts);
}

IpcClient::~IpcClient() {
  disconnect();
}

bool IpcClient::connect(const Poco::Net::SocketAddress& addr, std::chrono::milliseconds timeout) {
  {
    std::unique_lock<std::mutex> lk(_mu);
    if (_receiverThread.joinable()) {
      _receiverRunning = false;
      _connected = false;
      if (_sock.impl()) {
        try { _sock.shutdown(); } catch (...) {}
        _sock.close();
      }
      lk.unlock();
      _receiverThread.join();
      lk.lock();
      std::lock_guard<std::mutex> qlk(_queueMu);
      while (!_pongQueue.empty()) _pongQueue.pop();
      while (!_algoResultQueue.empty()) _algoResultQueue.pop();
    } else if (_sock.impl()) {
      _sock.close();
    }
  }

  std::lock_guard<std::mutex> lk(_mu);
  try {
    Poco::Net::StreamSocket newSocket;
    Poco::Timespan connectTs(static_cast<long>(timeout.count() / 1000),
                             static_cast<long>((timeout.count() % 1000) * 1000));
    newSocket.connect(addr, connectTs);
    ApplyTimeout(newSocket, timeout);

    _sock = newSocket;
    _connected = true;
    _receiverRunning = true;
    _receiverThread = std::thread(&IpcClient::receiverLoop, this);
    return true;
  } catch (const Poco::Exception& e) {
    _connected = false;
    common::log::Error("ipc", std::string("connect failed: ") + e.displayText());
    return false;
  }
}

void IpcClient::disconnect() {
  _receiverRunning = false;
  _connected = false;
  {
    std::lock_guard<std::mutex> lk(_mu);
    try {
      if (_sock.impl()) {
        _sock.shutdown();
        _sock.close();
      }
    } catch (...) {
    }
  }
  if (_receiverThread.joinable()) {
    _receiverThread.join();
  }
  {
    std::lock_guard<std::mutex> lk(_queueMu);
    while (!_pongQueue.empty()) _pongQueue.pop();
    while (!_algoResultQueue.empty()) _algoResultQueue.pop();
    _pongCv.notify_all();
    _algoResultCv.notify_all();
  }
}

bool IpcClient::isConnected() const {
  return _connected.load();
}

void IpcClient::receiverLoop() {
  common::log::SetThreadName("ipc-recv");
  constexpr std::chrono::milliseconds kRecvTimeout{200};

  while (_receiverRunning.load() && _connected.load()) {
    FrameHeader header;
    std::string payload;
    {
      std::lock_guard<std::mutex> lk(_mu);
      if (!_connected.load()) break;
      if (!receiveOneFrameLocked(header, payload)) {
        // Timeout or no data yet is normal; only exit when disconnect() closed the socket.
        continue;
      }
    }

    const MsgType type = static_cast<MsgType>(header.type);
    if (type == MsgType::Pong && header.payloadSize == sizeof(Pong)) {
      try {
        std::istringstream is(payload, std::ios::binary);
        Poco::BinaryReader r(is, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER);
        Pong pong;
        ReadPayload(r, pong);
        std::lock_guard<std::mutex> lk(_queueMu);
        _pongQueue.push(pong);
        _pongCv.notify_one();
      } catch (...) {
      }
    } else if (type == MsgType::AlgoResult && header.payloadSize == sizeof(AlgoResult)) {
      try {
        std::istringstream is(payload, std::ios::binary);
        Poco::BinaryReader r(is, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER);
        AlgoResult result;
        ReadPayload(r, result);
        std::lock_guard<std::mutex> lk(_queueMu);
        _algoResultQueue.push(result);
        _algoResultCv.notify_one();
      } catch (...) {
      }
    }
  }
}

bool IpcClient::receiveOneFrameLocked(FrameHeader& header, std::string& payload) {
  if (!_sock.impl()) return false;
  try {
    constexpr std::chrono::milliseconds kRecvTimeout{200};
    ApplyTimeout(_sock, kRecvTimeout);
    Poco::Net::SocketStream ss(_sock);

    std::string headerBuf;
    headerBuf.resize(sizeof(FrameHeader));
    ss.read(headerBuf.data(), static_cast<std::streamsize>(headerBuf.size()));
    if (ss.gcount() != static_cast<std::streamsize>(headerBuf.size())) return false;

    std::istringstream hs(headerBuf, std::ios::binary);
    Poco::BinaryReader hr(hs, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER);
    header = ReadHeader(hr);

    payload.clear();
    payload.resize(header.payloadSize);
    if (header.payloadSize > 0) {
      ss.read(payload.data(), static_cast<std::streamsize>(payload.size()));
      if (ss.gcount() != static_cast<std::streamsize>(payload.size())) return false;
    }
    return true;
  } catch (const Poco::Exception& e) {
    common::log::Error("ipc", std::string("recv failed: ") + e.displayText());
    return false;
  }
}

bool IpcClient::sendPing(const Ping& ping, std::chrono::milliseconds timeout) {
  return sendFrame(MsgType::Ping, &ping, sizeof(Ping), timeout);
}

bool IpcClient::sendSensorFrame(const SensorFrame& frame, std::chrono::milliseconds timeout) {
  return sendFrame(MsgType::SensorFrame, &frame, sizeof(SensorFrame), timeout);
}

bool IpcClient::tryReceivePong(Pong& pong, std::chrono::milliseconds timeout) {
  if (!_connected.load()) return false;
  std::unique_lock<std::mutex> lk(_queueMu);
  _pongCv.wait_for(lk, timeout, [this] {
    return !_pongQueue.empty() || !_connected.load();
  });
  if (!_connected.load()) return false;
  if (_pongQueue.empty()) return false;
  pong = _pongQueue.front();
  _pongQueue.pop();
  return true;
}

bool IpcClient::tryReceiveAlgoResult(AlgoResult& result, std::chrono::milliseconds timeout) {
  if (!_connected.load()) return false;
  std::unique_lock<std::mutex> lk(_queueMu);
  _algoResultCv.wait_for(lk, timeout, [this] {
    return !_algoResultQueue.empty() || !_connected.load();
  });
  if (!_connected.load()) return false;
  if (_algoResultQueue.empty()) return false;
  result = _algoResultQueue.front();
  _algoResultQueue.pop();
  return true;
}

bool IpcClient::sendFrame(MsgType type, const void* payload, std::size_t payloadSize,
                          std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lk(_mu);
  if (!_connected.load()) return false;

  try {
    ApplyTimeout(_sock, timeout);
    Poco::Net::SocketStream ss(_sock);

    std::ostringstream os(std::ios::binary);
    Poco::BinaryWriter w(os, Poco::BinaryWriter::BIG_ENDIAN_BYTE_ORDER);

    FrameHeader h;
    h.type = static_cast<std::uint16_t>(type);

    if (type == MsgType::Ping) {
      h.payloadSize = static_cast<std::uint32_t>(sizeof(Ping));
      WriteHeader(w, h);
      WritePayload(w, *static_cast<const Ping*>(payload));
    } else if (type == MsgType::SensorFrame) {
      h.payloadSize = static_cast<std::uint32_t>(sizeof(SensorFrame));
      WriteHeader(w, h);
      WritePayload(w, *static_cast<const SensorFrame*>(payload));
    } else {
      return false;
    }

    const auto& bytes = os.str();
    ss.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    ss.flush();
    return true;
  } catch (const Poco::Exception& e) {
    _connected = false;
    common::log::Error("ipc", std::string("send failed: ") + e.displayText());
    return false;
  }
}

} // namespace common::ipc
