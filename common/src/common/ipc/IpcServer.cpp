#include "common/ipc/IpcServer.h"

#include "common/ipc/BinaryCodec.h"
#include "common/log/Log.h"
#include "common/time/MonotonicClock.h"

#include <Poco/Exception.h>
#include <Poco/Net/SocketStream.h>

#include <sstream>

namespace common::ipc {

static void ApplyTimeout(Poco::Net::StreamSocket& sock, std::chrono::milliseconds timeout) {
  Poco::Timespan ts(static_cast<long>(timeout.count() / 1000),
                    static_cast<long>((timeout.count() % 1000) * 1000));
  sock.setReceiveTimeout(ts);
  sock.setSendTimeout(ts);
}

IpcServer::IpcServer(const Poco::Net::SocketAddress& bindAddr)
    : _bind(bindAddr), _srv(bindAddr, true) {}

IpcServer::~IpcServer() {
  stop();
}

bool IpcServer::start() {
  return true;
}

void IpcServer::stop() {
  _receiverRunning = false;
  _connected = false;
  {
    std::lock_guard<std::mutex> lk(_mu);
    try {
      if (_client.impl()) {
        _client.shutdown();
        _client.close();
      }
    } catch (...) {
    }
  }
  if (_receiverThread.joinable()) {
    _receiverThread.join();
  }
  {
    std::lock_guard<std::mutex> lk(_queueMu);
    while (!_pingQueue.empty()) _pingQueue.pop();
    while (!_sensorFrameQueue.empty()) _sensorFrameQueue.pop();
    _pingCv.notify_all();
    _sensorFrameCv.notify_all();
  }
}

bool IpcServer::acceptOne(std::chrono::milliseconds timeout) {
  {
    std::unique_lock<std::mutex> lk(_mu);
    if (_receiverThread.joinable()) {
      _receiverRunning = false;
      _connected = false;
      if (_client.impl()) {
        try { _client.shutdown(); } catch (...) {}
        _client.close();
      }
      lk.unlock();
      _receiverThread.join();
      lk.lock();
      std::lock_guard<std::mutex> qlk(_queueMu);
      while (!_pingQueue.empty()) _pingQueue.pop();
      while (!_sensorFrameQueue.empty()) _sensorFrameQueue.pop();
    }
  }

  std::lock_guard<std::mutex> lk(_mu);
  try {
    _srv.setReceiveTimeout(Poco::Timespan(static_cast<long>(timeout.count() / 1000),
                                          static_cast<long>((timeout.count() % 1000) * 1000)));
    _client = _srv.acceptConnection();
    _connected = true;
    _receiverRunning = true;
    _receiverThread = std::thread(&IpcServer::receiverLoop, this);
    return true;
  } catch (const Poco::Exception& e) {
    common::log::Warn("ipc", std::string("accept failed: ") + e.displayText());
    _connected = false;
    return false;
  }
}

bool IpcServer::isConnected() const {
  return _connected.load();
}

void IpcServer::receiverLoop() {
  constexpr std::chrono::milliseconds kRecvTimeout{200};

  while (_receiverRunning.load() && _connected.load()) {
    FrameHeader header;
    std::string payload;
    {
      std::lock_guard<std::mutex> lk(_mu);
      if (!_connected.load()) break;
      if (!receiveOneFrameLocked(header, payload)) {
        continue;
      }
    }

    const MsgType type = static_cast<MsgType>(header.type);
    if (type == MsgType::Ping && header.payloadSize == sizeof(Ping)) {
      try {
        std::istringstream is(payload, std::ios::binary);
        Poco::BinaryReader r(is, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER);
        Ping ping;
        ReadPayload(r, ping);
        // Reply to Ping immediately in receiver thread so Pong is never delayed behind SensorFrame processing.
        Pong pong;
        pong.seq = ping.seq;
        pong.t0MonotonicNs = ping.t0MonotonicNs;
        pong.t1MonotonicNs = common::time::NowMonotonicNs();
        (void)sendPong(pong, std::chrono::milliseconds(50));
      } catch (...) {
      }
    } else if (type == MsgType::SensorFrame && header.payloadSize == sizeof(SensorFrame)) {
      try {
        std::istringstream is(payload, std::ios::binary);
        Poco::BinaryReader r(is, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER);
        SensorFrame frame;
        ReadPayload(r, frame);
        std::lock_guard<std::mutex> lk(_queueMu);
        _sensorFrameQueue.push(frame);
        _sensorFrameCv.notify_one();
      } catch (...) {
      }
    }
  }
}

bool IpcServer::receiveOneFrameLocked(FrameHeader& header, std::string& payload) {
  if (!_client.impl()) return false;
  try {
    constexpr std::chrono::milliseconds kRecvTimeout{200};
    ApplyTimeout(_client, kRecvTimeout);
    Poco::Net::SocketStream ss(_client);

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

bool IpcServer::tryReceivePing(Ping& ping, std::chrono::milliseconds timeout) {
  if (!_connected.load()) return false;
  std::unique_lock<std::mutex> lk(_queueMu);
  _pingCv.wait_for(lk, timeout, [this] {
    return !_pingQueue.empty() || !_connected.load();
  });
  if (!_connected.load()) return false;
  if (_pingQueue.empty()) return false;
  ping = _pingQueue.front();
  _pingQueue.pop();
  return true;
}

bool IpcServer::tryReceiveSensorFrame(SensorFrame& frame, std::chrono::milliseconds timeout) {
  if (!_connected.load()) return false;
  std::unique_lock<std::mutex> lk(_queueMu);
  _sensorFrameCv.wait_for(lk, timeout, [this] {
    return !_sensorFrameQueue.empty() || !_connected.load();
  });
  if (!_connected.load()) return false;
  if (_sensorFrameQueue.empty()) return false;
  frame = _sensorFrameQueue.front();
  _sensorFrameQueue.pop();
  return true;
}

bool IpcServer::sendPong(const Pong& pong, std::chrono::milliseconds timeout) {
  return sendFrame(MsgType::Pong, &pong, sizeof(Pong), timeout);
}

bool IpcServer::sendAlgoResult(const AlgoResult& result, std::chrono::milliseconds timeout) {
  return sendFrame(MsgType::AlgoResult, &result, sizeof(AlgoResult), timeout);
}

bool IpcServer::sendFrame(MsgType type, const void* payload, std::size_t payloadSize,
                          std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lk(_mu);
  if (!_connected.load()) return false;

  try {
    ApplyTimeout(_client, timeout);
    Poco::Net::SocketStream ss(_client);

    std::ostringstream os(std::ios::binary);
    Poco::BinaryWriter w(os, Poco::BinaryWriter::BIG_ENDIAN_BYTE_ORDER);

    FrameHeader h;
    h.type = static_cast<std::uint16_t>(type);

    if (type == MsgType::Pong) {
      h.payloadSize = static_cast<std::uint32_t>(sizeof(Pong));
      WriteHeader(w, h);
      WritePayload(w, *static_cast<const Pong*>(payload));
    } else if (type == MsgType::AlgoResult) {
      h.payloadSize = static_cast<std::uint32_t>(sizeof(AlgoResult));
      WriteHeader(w, h);
      WritePayload(w, *static_cast<const AlgoResult*>(payload));
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
