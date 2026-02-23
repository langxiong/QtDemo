#pragma once

#include "cef_api/ApplicationHandler.hpp"
#include "cef_api/StreamHandler.hpp"
#include "cef_api/dto/ApiEnvelope.hpp"
#include <nlohmann/json.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cef_api {

/** Optional handler: returns result if it handles the request, else nullopt. */
using HandlerFn = std::function<std::optional<std::string>(const std::string& method, const nlohmann::json& params)>;

/** Observer: notified after processing; cannot change result. */
using ObserverFn = std::function<void(const std::string& type, const std::string& method, const nlohmann::json& params, const std::string& result)>;

class APIInterface {
public:
  APIInterface();

  /** Process request envelope. For type=notify: dispatch and return empty string.
   *  For type=call: dispatch and return response JSON (with callId). */
  std::string process(const std::string& requestJson);

  /** Add a handler. Handlers are tried before built-in route; first non-empty return wins. */
  void addHandler(HandlerFn handler);

  /** Add an observer. Observers are notified after processing (call and notify). */
  void addObserver(ObserverFn observer);

  /** Optional: hook for ControlLoop integration. Called when app start/stop (true/false). Run ControlLoop in dedicated thread to avoid blocking CEF message loop. */
  void setOnApplicationRunningChange(std::function<void(bool running)> f);

  /** Set optional backend. When set, process() forwards to backend first; on empty response, falls back to local handlers. */
  void setBackend(std::function<std::string(const std::string&)> backend);

private:
  std::string route(const std::string& method, const nlohmann::json& params);
  std::string makeError(const std::string& callId, const std::string& message);

  std::unique_ptr<ApplicationHandler> appHandler_;
  std::unique_ptr<StreamHandler> streamHandler_;
  std::vector<HandlerFn> handlers_;
  std::vector<ObserverFn> observers_;
  std::function<std::string(const std::string&)> backend_;
};

}  // namespace cef_api
