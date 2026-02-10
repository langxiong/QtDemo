#include "common/log/Log.h"

#include <Poco/AutoPtr.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/Logger.h>
#include <Poco/PatternFormatter.h>
#include <Poco/Thread.h>

#include <mutex>
#include <unordered_map>

namespace common::log {

namespace {
std::string gProcessName = "process";
Poco::Logger* gLogger = nullptr;
std::once_flag gOnceFlag;

std::string& ThreadNameSlot() {
  static thread_local std::string name;
  return name;
}

void InitPocoLogger() {
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

void SetThreadName(const std::string& name) {
  ThreadNameSlot() = name;
}

void Write(Level level, const std::string& module, const std::string& message) {
  auto& lg = GetLogger();
  lg.log(Poco::Message(lg.name(), MakePrefix(module) + message, ToPoco(level)));
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
