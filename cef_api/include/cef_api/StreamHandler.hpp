#pragma once

#include "cef_api/dto/StreamData.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace cef_api {

class StreamHandler {
public:
  /** Returns StreamData as JSON string, or error JSON if streamId not found. */
  std::string read(int streamId) const;

  /** Parses StreamData from JSON, executes stream command, returns status JSON. */
  std::string write(const std::string& json);

private:
  mutable dto::StreamData stream_{1, "", 0};
};

}  // namespace cef_api
