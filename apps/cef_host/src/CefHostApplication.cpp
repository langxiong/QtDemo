#include "CefHostApplication.hpp"
#include "cef_host/cef_host_app.hpp"
#include "cef_api/APIInterface.hpp"
#include "cef_api/IpcBackend.hpp"
#include "common/log/Log.h"
#include <include/cef_app.h>
#include <include/cef_command_line.h>
#include <cstdlib>
#include <memory>
#include <string>

#ifdef _WIN32
#include <include/cef_sandbox_win.h>
#include <windows.h>
#endif

static std::shared_ptr<cef_api::APIInterface> createApiInterface() {
  auto api = std::make_shared<cef_api::APIInterface>();
  const char* addr = std::getenv("MRCD_CONTROLLER_IPC_ADDR");
  if (!addr) addr = std::getenv("CONTROLLER_IPC_ADDR");
  if (addr && addr[0]) {
    std::string s(addr);
    size_t colon = s.find(':');
    std::string host = (colon != std::string::npos) ? s.substr(0, colon) : "127.0.0.1";
    int port = (colon != std::string::npos) ? std::stoi(s.substr(colon + 1)) : 9123;
    auto backend = cef_api::IpcBackend::create(host, port);
    if (backend) {
      auto shared_backend = std::shared_ptr<cef_api::IpcBackend>(std::move(backend));
      api->setBackend([shared_backend](const std::string& req) { return shared_backend->process(req); });
      common::log::Info("cef_host", "IPC backend connected to " + host + ":" + std::to_string(port));
    }
  }
  return api;
}

CefHostApplication::CefHostApplication() {
  setUnixOptions(true);
}

void CefHostApplication::initialize(Poco::Util::Application& self) {
  loadConfiguration();
  Application::initialize(self);
  common::log::Init("cef_host");
}

void CefHostApplication::defineOptions(Poco::Util::OptionSet& options) {
  Application::defineOptions(options);
  options.addOption(
      Poco::Util::Option("parent-hwnd", "p", "Parent HWND for SetAsChild embedding")
          .required(false)
          .repeatable(false)
          .argument("hwnd")
          .callback(Poco::Util::OptionCallback<CefHostApplication>(this, &CefHostApplication::handleOption)));
}

void CefHostApplication::handleOption(const std::string& name, const std::string& value) {
  if (name == "parent-hwnd") {
    if (!value.empty()) {
      _parentHwnd = static_cast<uintptr_t>(std::stoul(value, nullptr, 0));
    }
    return;
  }
  Application::handleOption(name, value);
}

int CefHostApplication::main(const std::vector<std::string>& args) {
  (void)args;

#ifdef _WIN32
  void* sandbox_info = nullptr;
  CefMainArgs main_args(::GetModuleHandle(nullptr));

  CefRefPtr<cef_host::CefHostApp> app(new cef_host::CefHostApp(createApiInterface()));
  if (_parentHwnd) {
    cef_host::ParentWindowInfo info;
    info.hwnd = reinterpret_cast<void*>(_parentHwnd);
    info.x = 0;
    info.y = 0;
    info.width = 800;
    info.height = 600;
    app->setParentWindow(info);
  }

  int exit_code = CefExecuteProcess(main_args, app, sandbox_info);
  if (exit_code >= 0) return exit_code;

  CefRefPtr<CefCommandLine> cmd = CefCommandLine::CreateCommandLine();
  cmd->InitFromString(::GetCommandLineW());

  CefSettings settings;
  settings.windowless_rendering_enabled = 0;
  settings.multi_threaded_message_loop = 0;
  settings.external_message_pump = 0;
  settings.no_sandbox = (sandbox_info == nullptr);

  if (!CefInitialize(main_args, settings, app, sandbox_info)) {
    common::log::Error("cef_host", "CefInitialize failed");
    return CefGetExitCode();
  }

  common::log::Info("cef_host", "CEF running. Start frontend: cd demo/frontend && npm run dev");

  CefRunMessageLoop();

  CefShutdown();

  return 0;
#else
  (void)_parentHwnd;
  common::log::Error("cef_host", "CefHostApplication GUI is Windows-only");
  return 1;
#endif
}
