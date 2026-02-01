#pragma once

#include <string>

namespace common::process {

class ProcessLauncher {
public:
  // Starts a child process and returns true if a process was started.
  // For demo purposes, this is a minimal abstraction.
  static bool startDetached(const std::string& executablePath, const std::string& args);
};

} // namespace common::process
