#include "IpcServer.h"
#include "DeviceSimulator.h"
#include "common/config/ConfigPoco.h"
#include "common/log/Log.h"
#include "cef_api/dto/ApplicationData.hpp"
#include "cef_api/dto/StreamData.hpp"

#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketAcceptor.h>
#include <Poco/Net/TCPServerConnection.h>
#include <Poco/Net/TCPServerConnectionFactory.h>
#include <Poco/Net/TCPServer.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <sstream>
#include <string>

namespace controller_app {

namespace {

class ApiConnection : public Poco::Net::TCPServerConnection {
public:
  ApiConnection(const Poco::Net::StreamSocket& socket, DeviceSimulator* device,
               common::status::StatusStore* statusStore,
               const common::sensor::SensorPipeline* sensor)
      : TCPServerConnection(socket),
        _device(device),
        _statusStore(statusStore),
        _sensor(sensor) {}

  void run() override {
    Poco::Net::StreamSocket& socket = this->socket();
    socket.setReceiveTimeout(Poco::Timespan(30, 0));
    socket.setSendTimeout(Poco::Timespan(10, 0));

    std::string line;
    while (socket.poll(Poco::Timespan(1, 0), Poco::Net::Socket::SELECT_READ)) {
      char c;
      int n = socket.receiveBytes(&c, 1);
      if (n <= 0) break;
      if (c == '\n') {
        if (!line.empty()) {
          std::string resp = handleLine(line);
          if (!resp.empty()) {
            resp += '\n';
            socket.sendBytes(resp.data(), static_cast<int>(resp.size()));
          }
          line.clear();
        }
      } else {
        line += c;
        if (line.size() > 1024 * 1024) line.clear();
      }
    }
  }

private:
  std::string handleLine(const std::string& requestJson) {
    try {
      const auto j = nlohmann::json::parse(requestJson);
      const std::string type = j.value("type", "");
      const std::string method = j.value("method", "");
      const auto params = j.value("params", nlohmann::json::object());

      std::string result = route(method, params);

      if (type == "notify") return {};

      if (type == "call") {
        const std::string callId = j.value("callId", "");
        if (result.empty()) {
          return nlohmann::json{{"callId", callId}, {"result", {{"error_code", -1}, {"error", "empty result"}}}}.dump();
        }
        const auto parsed = nlohmann::json::parse(result);
        return nlohmann::json{{"callId", callId}, {"result", parsed}}.dump();
      }
      return nlohmann::json{{"error_code", -1}, {"error", "unknown type"}}.dump();
    } catch (const std::exception& e) {
      common::log::Error("ipc", std::string("handleRequest: ") + e.what());
      return nlohmann::json{{"error_code", -1}, {"error", e.what()}}.dump();
    }
  }

  std::string route(const std::string& method, const nlohmann::json& params) {
    if (method == "readApp") {
      const int id = params.value("id", 1);
      cef_api::dto::ApplicationData app;
      app.id = id;
      app.name = "controller_app";
      app.status = _device && _device->isRunning() ? "running" : "stopped";
      nlohmann::json j;
      cef_api::dto::to_json(j, app);
      j["error_code"] = 0;
      return j.dump();
    }
    if (method == "writeApp") {
      const std::string status = params.value("status", "");
      if (_device) {
        if (status == "start") {
          _device->start();
          return nlohmann::json{{"error_code", 0}}.dump();
        }
        if (status == "stop" || status == "reset") {
          _device->stop();
          return nlohmann::json{{"error_code", 0}}.dump();
        }
      }
      return nlohmann::json{{"error_code", -1}, {"error", "unknown command"}}.dump();
    }
    if (method == "readStream") {
      const int streamId = params.value("streamId", params.value("id", 1));
      cef_api::dto::StreamData stream;
      stream.streamId = streamId;
      if (_sensor) {
        const auto snap = _sensor->latest();
        nlohmann::json payload;
        payload["seq"] = snap.latest.seq;
        payload["valueA"] = snap.latest.valueA;
        payload["valueB"] = snap.latest.valueB;
        payload["valueC"] = snap.latest.valueC;
        payload["effectiveRateHz"] = snap.effectiveRateHz;
        if (_statusStore) {
          const auto st = _statusStore->read();
          payload["lastCommand"] = st.lastCommand;
          payload["actuatorPosition"] = st.actuatorPosition;
          payload["actuatorVelocity"] = st.actuatorVelocity;
        }
        stream.payload = payload.dump();
      }
      stream.timestamp = static_cast<std::int64_t>(
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count());
      nlohmann::json j;
      cef_api::dto::to_json(j, stream);
      j["error_code"] = 0;
      return j.dump();
    }
    return nlohmann::json{{"error_code", -1}, {"error", "unknown method: " + method}}.dump();
  }

  DeviceSimulator* _device;
  common::status::StatusStore* _statusStore;
  const common::sensor::SensorPipeline* _sensor;
};

class ApiConnectionFactory : public Poco::Net::TCPServerConnectionFactory {
public:
  ApiConnectionFactory(DeviceSimulator* device, common::status::StatusStore* statusStore,
                      const common::sensor::SensorPipeline* sensor)
      : _device(device), _statusStore(statusStore), _sensor(sensor) {}

  Poco::Net::TCPServerConnection* createConnection(const Poco::Net::StreamSocket& socket) override {
    return new ApiConnection(socket, _device, _statusStore, _sensor);
  }

private:
  DeviceSimulator* _device;
  common::status::StatusStore* _statusStore;
  const common::sensor::SensorPipeline* _sensor;
};

}  // namespace

IpcServer::IpcServer(const common::config::Config& cfg, DeviceSimulator& device,
                     common::status::StatusStore& statusStore,
                     const common::sensor::SensorPipeline& sensor)
    : _cfg(cfg), _device(device), _statusStore(statusStore), _sensor(sensor) {
  _listenPort = _cfg.getInt("ipc.api_port", 9123);
}

IpcServer::~IpcServer() { stop(); }

void IpcServer::start() {
  if (_running.exchange(true)) return;
  _thread = std::thread(&IpcServer::run, this);
}

void IpcServer::stop() {
  if (!_running.exchange(false)) return;
  if (_thread.joinable()) _thread.join();
}

void IpcServer::run() {
  try {
    Poco::Net::ServerSocket serverSocket(_listenPort);
    serverSocket.listen();
    Poco::Net::TCPServer server(
        new ApiConnectionFactory(&_device, &_statusStore, &_sensor), serverSocket);
    server.start();
    common::log::Info("ipc", "IPC server listening on port " + std::to_string(_listenPort));

    while (_running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    server.stop();
  } catch (const std::exception& e) {
    common::log::Error("ipc", std::string("IPC server: ") + e.what());
  }
}

}  // namespace controller_app
