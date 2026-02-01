#include "common/ipc/IpcServer.h"

#include "common/ipc/BinaryCodec.h"
#include "common/log/Log.h"

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

bool IpcServer::start() {
  return true;
}

void IpcServer::stop() {
  std::lock_guard<std::mutex> lk(_mu);
  try {
    if (_connected) {
      _client.shutdown();
      _client.close();
    }
  } catch (...) {
  }
  _connected = false;
}

bool IpcServer::acceptOne(std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lk(_mu);
  try {
    _srv.setReceiveTimeout(Poco::Timespan(static_cast<long>(timeout.count() / 1000),
                                          static_cast<long>((timeout.count() % 1000) * 1000)));
    _client = _srv.acceptConnection();
    _connected = true;
    return true;
  } catch (const Poco::Exception& e) {
    common::log::Log::Warn("ipc", std::string("accept failed: ") + e.displayText());
    _connected = false;
    return false;
  }
}

bool IpcServer::isConnected() const {
  std::lock_guard<std::mutex> lk(_mu);
  return _connected;
}

bool IpcServer::tryReceivePing(Ping& ping, std::chrono::milliseconds timeout) {
  FrameHeader h;
  std::string payload;
  if (!receiveFrame(h, payload, timeout)) return false;
  if (static_cast<MsgType>(h.type) != MsgType::Ping) return false;

  std::istringstream is(payload, std::ios::binary);
  Poco::BinaryReader r(is, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER);
  ReadPayload(r, ping);
  return true;
}

bool IpcServer::tryReceiveSensorFrame(SensorFrame& frame, std::chrono::milliseconds timeout) {
  FrameHeader h;
  std::string payload;
  if (!receiveFrame(h, payload, timeout)) return false;
  if (static_cast<MsgType>(h.type) != MsgType::SensorFrame) return false;

  std::istringstream is(payload, std::ios::binary);
  Poco::BinaryReader r(is, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER);
  ReadPayload(r, frame);
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
  if (!_connected) return false;

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
    common::log::Log::Error("ipc", std::string("send failed: ") + e.displayText());
    return false;
  }
}

bool IpcServer::receiveFrame(FrameHeader& header, std::string& payload, std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lk(_mu);
  if (!_connected) return false;

  try {
    ApplyTimeout(_client, timeout);
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
    _connected = false;
    common::log::Log::Error("ipc", std::string("recv failed: ") + e.displayText());
    return false;
  }
}

} // namespace common::ipc
