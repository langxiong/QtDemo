#include "cef_api/APIInterface.hpp"
#include "common/log/Log.h"
#include <nlohmann/json.hpp>

namespace cef_api {

APIInterface::APIInterface()
    : appHandler_(std::make_unique<ApplicationHandler>()),
      streamHandler_(std::make_unique<StreamHandler>()) {}

void APIInterface::setBackend(std::function<std::string(const std::string&)> backend) {
  backend_ = std::move(backend);
}

void APIInterface::addHandler(HandlerFn handler) {
  handlers_.push_back(std::move(handler));
}

void APIInterface::addObserver(ObserverFn observer) {
  observers_.push_back(std::move(observer));
}

void APIInterface::setOnApplicationRunningChange(std::function<void(bool running)> f) {
  appHandler_->setOnRunningChange(std::move(f));
}

std::string APIInterface::process(const std::string& requestJson) {
  try {
    const auto j = nlohmann::json::parse(requestJson);
    const std::string type = j.value("type", "");
    const std::string method = j.value("method", "");

    if (backend_) {
      std::string resp = backend_(requestJson);
      if (!resp.empty()) {
        for (const auto& obs : observers_) {
          try {
            const auto params = j.value("params", nlohmann::json::object());
            obs(type, method, params, resp);
          } catch (const std::exception& e) {
            common::log::Error("cef_api", std::string("APIInterface observer: ") + e.what());
          }
        }
        if (type == "notify") return {};
        return resp;
      }
    }

    const auto params = j.value("params", nlohmann::json::object());
    std::string result = route(method, params);

    // Notify observers (for both call and notify)
    for (const auto& obs : observers_) {
      try {
        obs(type, method, params, result);
      } catch (const std::exception& e) {
        common::log::Error("cef_api", std::string("APIInterface observer: ") + e.what());
      }
    }

    if (type == "notify") {
      return {};
    }
    if (type == "call") {
      const std::string callId = j.value("callId", "");
      if (result.empty()) {
        return makeError(callId, "empty result");
      }
      const auto parsed = nlohmann::json::parse(result);
      nlohmann::json resp;
      resp["callId"] = callId;
      resp["result"] = parsed;
      return resp.dump();
    }
    return makeError("", "unknown type: " + type);
  } catch (const nlohmann::json::exception& e) {
    common::log::Error("cef_api", std::string("APIInterface::process json: ") + e.what());
    return makeError("", std::string(e.what()));
  } catch (const std::exception& e) {
    common::log::Error("cef_api", std::string("APIInterface::process: ") + e.what());
    return makeError("", std::string(e.what()));
  }
}

std::string APIInterface::route(const std::string& method, const nlohmann::json& params) {
  // Try custom handlers first
  for (const auto& h : handlers_) {
    try {
      auto r = h(method, params);
      if (r.has_value() && !r->empty()) {
        return *r;
      }
    } catch (const std::exception& e) {
      common::log::Error("cef_api", std::string("APIInterface handler: ") + e.what());
    }
  }

  // Built-in routing
  if (method == "readApp") {
    const int id = params.value("id", 1);
    return appHandler_->read(id);
  }
  if (method == "writeApp") {
    return appHandler_->write(params.dump());
  }
  if (method == "readStream") {
    const int streamId = params.value("streamId", params.value("id", 1));
    return streamHandler_->read(streamId);
  }
  if (method == "writeStream") {
    return streamHandler_->write(params.dump());
  }
  return nlohmann::json{{"error_code", -1}, {"error", "unknown method: " + method}}.dump();
}

std::string APIInterface::makeError(const std::string& callId, const std::string& message) {
  return nlohmann::json{
    {"callId", callId},
    {"result", {{"error_code", -1}, {"error", message}}},
  }.dump();
}

}  // namespace cef_api
