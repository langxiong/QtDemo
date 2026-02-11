#include "common/log/Log.h"

#include <Poco/AutoPtr.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/Logger.h>
#include <Poco/PatternFormatter.h>
#include <Poco/Thread.h>
#include <Poco/Util/AbstractConfiguration.h>

#include <mutex>
#include <unordered_map>

namespace common::log {

namespace {
std::string gProcessName = "process";
Poco::Logger* gLogger = nullptr;
std::once_flag gOnceFlag;
Sink gSink;
std::mutex gSinkMu;

std::string& ThreadNameSlot() {
  static thread_local std::string name;
  return name;
}

void InitPocoLogger() {
    if (gLogger != nullptr) return;  // Already configured by InitFromConfig
    Poco::AutoPtr<Poco::PatternFormatter> pf(new Poco::PatternFormatter);
    pf->setProperty("pattern", "%Y-%m-%d %H:%M:%S.%i [%p][%s] %t");
    pf->setProperty("times", "local");

    Poco::AutoPtr<Poco::ConsoleChannel> cc(new Poco::ConsoleChannel);
    Poco::AutoPtr<Poco::FormattingChannel> fc(new Poco::FormattingChannel(pf, cc));

    Poco::Logger::root().setChannel(fc);
    Poco::Logger::root().setLevel("information");

    gLogger = &Poco::Logger::get(gProcessName);
}

Poco::Message::Priority ToPoco(Level level) {
  switch (level) {
    case Level::Trace:
      return Poco::Message::PRIO_TRACE;
    case Level::Debug:
      return Poco::Message::PRIO_DEBUG;
    case Level::Info:
      return Poco::Message::PRIO_INFORMATION;
    case Level::Warn:
      return Poco::Message::PRIO_WARNING;
    case Level::Error:
      return Poco::Message::PRIO_ERROR;
    case Level::Fatal:
      return Poco::Message::PRIO_FATAL;
  }
  return Poco::Message::PRIO_INFORMATION;
}

Poco::Logger& GetLogger() {
  std::call_once(gOnceFlag, InitPocoLogger);
  return *gLogger;
}

std::string MakePrefix(const std::string& module) {
  const auto tid = Poco::Thread::currentTid();
  const auto& tn = ThreadNameSlot();
  std::string thread = tn.empty() ? std::to_string(tid) : tn;
  return "[" + module + "][" + thread + "] ";
}

} // namespace

void Init(const std::string& processName) {
  gProcessName = processName;
  (void)GetLogger(); // Eagerly initialize
}

void InitFromConfig(Poco::Util::AbstractConfiguration& cfg, const std::string& loggerName) {
  gProcessName = loggerName;
  const std::string pattern = cfg.getString("logging.pattern", "%Y-%m-%d %H:%M:%S.%i [%p][%s] %t");
  const std::string level = cfg.getString("logging.level", "information");
  const std::string channel = cfg.getString("logging.channel", "console");
  const std::string filePath = cfg.getString("logging.file", "logs/app.log");

  Poco::AutoPtr<Poco::PatternFormatter> pf(new Poco::PatternFormatter);
  pf->setProperty("pattern", pattern);
  pf->setProperty("times", "local");

  Poco::AutoPtr<Poco::Channel> ch;
  if (channel == "file") {
    ch = new Poco::FileChannel(filePath);
  } else {
    ch = new Poco::ConsoleChannel;
  }

  Poco::AutoPtr<Poco::FormattingChannel> fc(new Poco::FormattingChannel(pf, ch));
  Poco::Logger::root().setChannel(fc);
  Poco::Logger::root().setLevel(level);

  gLogger = &Poco::Logger::get(loggerName);
}

void SetThreadName(const std::string& name) {
  ThreadNameSlot() = name;
}

void SetSink(Sink sink) {
  std::lock_guard<std::mutex> lk(gSinkMu);
  gSink = std::move(sink);
}

void Write(Level level, const std::string& module, const std::string& message) {
  auto& lg = GetLogger();
  lg.log(Poco::Message(lg.name(), MakePrefix(module) + message, ToPoco(level)));
  Sink copy;
  {
    std::lock_guard<std::mutex> lk(gSinkMu);
    copy = gSink;
  }
  if (copy)
    copy(level, module, message);
}

void Trace(const std::string& module, const std::string& message) { Write(Level::Trace, module, message); }
void Debug(const std::string& module, const std::string& message) { Write(Level::Debug, module, message); }
void Info(const std::string& module, const std::string& message) { Write(Level::Info, module, message); }
void Warn(const std::string& module, const std::string& message) { Write(Level::Warn, module, message); }
void Error(const std::string& module, const std::string&message) { Write(Level::Error, module, message); }
void Fatal(const std::string& module, const std::string& message) { Write(Level::Fatal, module, message); }

std::string LevelToString(Level level) {
  switch (level) {
    case Level::Trace:
      return "TRACE";
    case Level::Debug:
      return "DEBUG";
    case Level::Info:
      return "INFO";
    case Level::Warn:
      return "WARN";
    case Level::Error:
      return "ERROR";
    case Level::Fatal:
      return "FATAL";
  }
  return "INFO";
}

} // namespace common::log
