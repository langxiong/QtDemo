#pragma once

#include <Poco/Util/ServerApplication.h>
#include <cstdint>

/** Poco Application for cef_host: parses --parent-hwnd, runs CEF. */
class CefHostApplication : public Poco::Util::ServerApplication {
public:
  CefHostApplication();

protected:
  void initialize(Poco::Util::Application& self) override;
  void defineOptions(Poco::Util::OptionSet& options) override;
  void handleOption(const std::string& name, const std::string& value) override;
  int main(const std::vector<std::string>& args) override;

private:
  uintptr_t _parentHwnd{0};
};
