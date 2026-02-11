#include "QtSubsystem.h"

#include <QApplication>
#include <Poco/Util/Application.h>

#include <vector>

void QtSubsystem::initialize(Poco::Util::Application& app) {
  const auto& args = app.argv();
  _argvStorage = args;
  _argvPtrs.clear();
  for (auto& s : _argvStorage) {
    _argvPtrs.push_back(const_cast<char*>(s.c_str()));
  }
  _argvPtrs.push_back(nullptr);

  int argc = static_cast<int>(_argvPtrs.size()) - 1;
  char** argv = _argvPtrs.empty() ? nullptr : _argvPtrs.data();

  _app = std::make_unique<QApplication>(argc, argv);
}

void QtSubsystem::uninitialize() {
  _app.reset();
  _argvPtrs.clear();
  _argvStorage.clear();
}
