#pragma once

#include <Poco/Util/OptionSet.h>
#include <Poco/Util/ServerApplication.h>
#include <memory>

#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)
namespace controller_app { class UISubsystem; }
#endif

class ControllerApp : public Poco::Util::ServerApplication {
public:
  ControllerApp();
  ~ControllerApp();

protected:
  void initialize(Poco::Util::Application& self) override;
  void defineOptions(Poco::Util::OptionSet& options) override;
  void handleOption(const std::string& name, const std::string& value) override;
  int main(const std::vector<std::string>& args) override;

private:
  bool _verifyStartup{false};

#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)
  std::unique_ptr<controller_app::UISubsystem> _ui;
#endif
};
