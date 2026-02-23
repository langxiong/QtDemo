#include "cef_api/IpcBackend.hpp"
#include "common/log/Log.h"
#include <nlohmann/json.hpp>

#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketStream.h>

#include <sstream>
#include <string>

namespace cef_api {

namespace {

std::string sendReceive(Poco::Net::StreamSocket& socket, const std::string& request) {
  std::string line = request;
  if (line.back() != '\n') line += '\n';
  socket.sendBytes(line.data(), static_cast<int>(line.size()));

  std::string response;
  char c;
  while (socket.poll(Poco::Timespan(5, 0), Poco::Net::Socket::SELECT_READ)) {
    int n = socket.receiveBytes(&c, 1);
    if (n <= 0) break;
    if (c == '\n') break;
    response += c;
    if (response.size() > 1024 * 1024) return {};
  }
  return response;
}

}  // namespace

std::unique_ptr<IpcBackend> IpcBackend::create(const std::string& host, int port) {
  try {
    auto socket = std::make_unique<Poco::Net::StreamSocket>();
    Poco::Net::SocketAddress addr(host, static_cast<Poco::UInt16>(port));
    socket->connect(addr);
    socket->setSendTimeout(Poco::Timespan(5, 0));
    socket->setReceiveTimeout(Poco::Timespan(10, 0));
    return std::unique_ptr<IpcBackend>(new IpcBackend(std::move(socket)));
  } catch (const std::exception& e) {
    common::log::Warn("cef_api", std::string("IpcBackend connect failed: ") + e.what());
    return nullptr;
  }
}

IpcBackend::IpcBackend(std::unique_ptr<Poco::Net::StreamSocket> socket) : _socket(std::move(socket)) {}

IpcBackend::~IpcBackend() = default;

std::string IpcBackend::process(const std::string& requestJson) {
  if (!_socket) return {};
  try {
    std::string resp = sendReceive(*_socket, requestJson);
    if (resp.empty()) return {};
    try {
      auto j = nlohmann::json::parse(resp);
      auto req = nlohmann::json::parse(requestJson);
      if (req.contains("callId") && j.contains("callId")) {
        j["callId"] = req["callId"];
        resp = j.dump();
      }
    } catch (...) {}
    return resp;
  } catch (const std::exception& e) {
    common::log::Error("cef_api", std::string("IpcBackend::process: ") + e.what());
    return {};
  }
}

}  // namespace cef_api
