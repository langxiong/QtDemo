#pragma once

#include "cef_api/dto/ApplicationData.hpp"
#include <functional>
#include <nlohmann/json.hpp>
#include <string>

namespace cef_api {

class ApplicationHandler {
public:
  /** Returns ApplicationData as JSON string, or error JSON if id not found. */
  std::string read(int id) const;

  /** Parses command from JSON, executes (start/stop/reset), returns status JSON. */
  std::string write(const std::string& json);

  /** Optional: called when running state changes (true=start, false=stop/reset). Use to start/stop ControlLoop. */
  void setOnRunningChange(std::function<void(bool running)> f) { onRunningChange_ = std::move(f); }

private:
  mutable dto::ApplicationData app_{1, "demo_app", "stopped"};
  std::function<void(bool)> onRunningChange_;
};

}  // namespace cef_api
