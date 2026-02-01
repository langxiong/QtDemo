#include "common/process/ProcessLauncher.h"

#include <Poco/Process.h>

#include <vector>

namespace common::process {

bool ProcessLauncher::startDetached(const std::string& executablePath, const std::string& args) {
  try {
    std::vector<std::string> argv;
    if (!args.empty()) argv.push_back(args);

    Poco::Process::launch(executablePath, argv, nullptr, nullptr, nullptr);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace common::process
