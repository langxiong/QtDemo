#pragma once

#include <string>

namespace common::log {

enum class Level {
  Trace,
  Debug,
  Info,
  Warn,
  Error,
  Fatal
};

struct Context {
  std::string process;
  std::string module;
  std::string thread;
};

class Log {
public:
  static void Init(const std::string& processName);
  static void SetThreadName(const std::string& name);

  static void Write(Level level, const std::string& module, const std::string& message);

  static void Trace(const std::string& module, const std::string& message);
  static void Debug(const std::string& module, const std::string& message);
  static void Info(const std::string& module, const std::string& message);
  static void Warn(const std::string& module, const std::string& message);
  static void Error(const std::string& module, const std::string& message);
  static void Fatal(const std::string& module, const std::string& message);

private:
  static std::string LevelToString(Level level);
};

} // namespace common::log
