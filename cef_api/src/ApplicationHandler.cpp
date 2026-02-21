#include "cef_api/ApplicationHandler.hpp"
#include "cef_api/dto/ApplicationData.hpp"
#include "common/log/Log.h"
#include <nlohmann/json.hpp>

namespace cef_api {

std::string ApplicationHandler::read(int id) const {
  try {
    if (id != app_.id) {
      const auto err = nlohmann::json{{"error_code", -1}, {"error", "unknown id"}}.dump();
      common::log::Warn("cef_api", "ApplicationHandler::read unknown id=" + std::to_string(id));
      return err;
    }
    nlohmann::json j;
    dto::to_json(j, app_);
    j["error_code"] = 0;
    return j.dump();
  } catch (const std::exception& e) {
    const auto err = nlohmann::json{{"error_code", -1}, {"error", e.what()}}.dump();
    common::log::Error("cef_api", std::string("ApplicationHandler::read: ") + e.what());
    return err;
  }
}

std::string ApplicationHandler::write(const std::string& json) {
  try {
    const auto j = nlohmann::json::parse(json);
    const std::string status = j.value("status", "");
    if (status == "start") {
      app_.status = "running";
      if (onRunningChange_) onRunningChange_(true);
      return nlohmann::json{{"error_code", 0}}.dump();
    }
    if (status == "stop") {
      app_.status = "stopped";
      if (onRunningChange_) onRunningChange_(false);
      return nlohmann::json{{"error_code", 0}}.dump();
    }
    if (status == "reset") {
      app_.status = "stopped";
      if (onRunningChange_) onRunningChange_(false);
      return nlohmann::json{{"error_code", 0}}.dump();
    }
    const auto err = nlohmann::json{{"error_code", -1}, {"error", "unknown command"}}.dump();
    common::log::Warn("cef_api", "ApplicationHandler::write unknown status=" + status);
    return err;
  } catch (const nlohmann::json::exception& e) {
    const auto err = nlohmann::json{{"error_code", -1}, {"error", e.what()}}.dump();
    common::log::Error("cef_api", std::string("ApplicationHandler::write json: ") + e.what());
    return err;
  }
}

}  // namespace cef_api
