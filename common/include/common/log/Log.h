#pragma once

#include <functional>
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

void Init(const std::string& processName);
void SetThreadName(const std::string& name);

void Write(Level level, const std::string& module, const std::string& message);

/** Optional sink: called from Write() on every log line (may be from any thread). Use for UI forwarding with QueuedConnection. */
using Sink = std::function<void(Level level, const std::string& module, const std::string& message)>;
void SetSink(Sink sink);

void Trace(const std::string& module, const std::string& message);
void Debug(const std::string& module, const std::string& message);
void Info(const std::string& module, const std::string& message);
void Warn(const std::string& module, const std::string& message);
void Error(const std::string& module, const std::string& message);
void Fatal(const std::string& module, const std::string& message);

std::string LevelToString(Level level);

} // namespace common::log
