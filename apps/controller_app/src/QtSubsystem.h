#pragma once

#include <Poco/Util/Subsystem.h>
#include <memory>
#include <string>
#include <vector>

class QApplication;

class QtSubsystem : public Poco::Util::Subsystem {
public:
  QtSubsystem() = default;
  ~QtSubsystem() override = default;

  const char* name() const override { return "QtSubsystem"; }

  void initialize(Poco::Util::Application& app) override;
  void uninitialize() override;

  QApplication* qApplication() const { return _app.get(); }

private:
  std::unique_ptr<QApplication> _app;
  std::vector<std::string> _argvStorage;
  std::vector<char*> _argvPtrs;
};
