#include "cef_api/StreamHandler.hpp"
#include "cef_api/dto/StreamData.hpp"
#include "common/log/Log.h"
#include <nlohmann/json.hpp>
#include <chrono>

namespace cef_api {

std::string StreamHandler::read(int streamId) const {
  try {
    if (streamId != stream_.streamId) {
      const auto err = nlohmann::json{{"error_code", -1}, {"error", "unknown streamId"}}.dump();
      common::log::Warn("cef_api", "StreamHandler::read unknown streamId=" + std::to_string(streamId));
      return err;
    }
    stream_.timestamp = static_cast<std::int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    nlohmann::json j;
    dto::to_json(j, stream_);
    j["error_code"] = 0;
    return j.dump();
  } catch (const std::exception& e) {
    const auto err = nlohmann::json{{"error_code", -1}, {"error", e.what()}}.dump();
    common::log::Error("cef_api", std::string("StreamHandler::read: ") + e.what());
    return err;
  }
}

std::string StreamHandler::write(const std::string& json) {
  try {
    const auto j = nlohmann::json::parse(json);
    dto::StreamData data;
    dto::from_json(j, data);
    stream_ = data;
    return nlohmann::json{{"error_code", 0}}.dump();
  } catch (const nlohmann::json::exception& e) {
    const auto err = nlohmann::json{{"error_code", -1}, {"error", e.what()}}.dump();
    common::log::Error("cef_api", std::string("StreamHandler::write json: ") + e.what());
    return err;
  }
}

}  // namespace cef_api
