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

bool IpcClient::connect(const Poco::Net::SocketAddress& addr, std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lk(_mu);
  try {
    _sock = Poco::Net::StreamSocket();
    ApplyTimeout(_sock, timeout);
    _sock.connect(addr, Poco::Timespan(static_cast<long>(timeout.count() / 1000),
                                       static_cast<long>((timeout.count() % 1000) * 1000)));
    _connected = true;
    return true;
  } catch (const Poco::Exception& e) {
    _connected = false;
    common::log::Log::Error("ipc", std::string("connect failed: ") + e.displayText());
    return false;
  }
}

void IpcClient::disconnect() {
  std::lock_guard<std::mutex> lk(_mu);
  try {
    if (_connected) {
      _sock.shutdown();
      _sock.close();
    }
  } catch (...) {
  }
  _connected = false;
}

bool IpcClient::isConnected() const {
  std::lock_guard<std::mutex> lk(_mu);
  return _connected;
}

bool IpcClient::sendPing(const Ping& ping, std::chrono::milliseconds timeout) {
  return sendFrame(MsgType::Ping, &ping, sizeof(Ping), timeout);
}

bool IpcClient::sendSensorFrame(const SensorFrame& frame, std::chrono::milliseconds timeout) {
  return sendFrame(MsgType::SensorFrame, &frame, sizeof(SensorFrame), timeout);
}

bool IpcClient::tryReceivePong(Pong& pong, std::chrono::milliseconds timeout) {
  FrameHeader h;
  std::string payload;
  if (!receiveFrame(h, payload, timeout)) return false;
  if (static_cast<MsgType>(h.type) != MsgType::Pong) return false;

  std::istringstream is(payload, std::ios::binary);
  Poco::BinaryReader r(is, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER);
  ReadPayload(r, pong);
  return true;
}

bool IpcClient::tryReceiveAlgoResult(AlgoResult& result, std::chrono::milliseconds timeout) {
  FrameHeader h;
  std::string payload;
  if (!receiveFrame(h, payload, timeout)) return false;
  if (static_cast<MsgType>(h.type) != MsgType::AlgoResult) return false;

  std::istringstream is(payload, std::ios::binary);
  Poco::BinaryReader r(is, Poco::BinaryReader::BIG_ENDIAN_BYTE_ORDER);
  ReadPayload(r, result);
  return true;
}

bool IpcClient::sendFrame(MsgType type, const void* payload, std::size_t payloadSize,
                          std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lk(_mu);
  if (!_connected) return false;

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
    common::log::Log::Error("ipc", std::string("send failed: ") + e.displayText());
    return false;
  }
}

bool IpcClient::receiveFrame(FrameHeader& header, std::string& payload, std::chrono::milliseconds timeout) {
  std::lock_guard<std::mutex> lk(_mu);
  if (!_connected) return false;

  try {
    ApplyTimeout(_sock, timeout);
    Poco::Net::SocketStream ss(_sock);

    // Read header (fixed size)
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
