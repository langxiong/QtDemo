/**
 * cef_host: CEF host for React UI.
 * With MRCD_CEF_ROOT: full CEF init, browser window, window.api bridge.
 * Without: stub, prints setup instructions.
 * Set MRCD_CONTROLLER_IPC_ADDR=127.0.0.1:9123 to use controller_app as API backend.
 */
#include "cef_api/APIInterface.hpp"
#include "cef_api/IpcBackend.hpp"
#include "common/log/Log.h"
#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(HAVE_CEF)
#include "cef_host/cef_host_app.hpp"
#include <include/cef_app.h>
#include <include/cef_command_line.h>
#include <include/cef_sandbox_win.h>
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
      api->setBackend([b = std::move(backend)](const std::string& req) { return b->process(req); });
      common::log::Info("cef_host", "IPC backend connected to " + host + ":" + std::to_string(port));
    }
  }
  return api;
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  common::log::Init("cef_host");

#if defined(HAVE_CEF)
  CefMainArgs main_args;
#ifdef _WIN32
  main_args = CefMainArgs(::GetModuleHandle(nullptr));
#else
  main_args = CefMainArgs(argc, argv);
#endif

  CefRefPtr<cef_host::CefHostApp> app(new cef_host::CefHostApp(createApiInterface()));
  int exit_code = CefExecuteProcess(main_args, app, nullptr);
  if (exit_code >= 0) return exit_code;

  CefRefPtr<CefCommandLine> cmd = CefCommandLine::CreateCommandLine();
#ifdef _WIN32
  cmd->InitFromString(::GetCommandLineW());
#else
  cmd->InitFromArgv(argc, argv);
#endif

  CefSettings settings;
  settings.windowless_rendering_enabled = 0;
  settings.multi_threaded_message_loop = 0;
  settings.external_message_pump = 0;
  settings.no_sandbox = 1;

  if (!CefInitialize(main_args, settings, app, nullptr)) {
    common::log::Error("cef_host", "CefInitialize failed");
    return 1;
  }

  common::log::Info("cef_host", "CEF running. Start frontend: cd demo/frontend && npm run dev");

  CefRunMessageLoop();

  CefShutdown();
  return 0;
#else
  common::log::Info("cef_host", "CEF stub. Set MRCD_CEF_ROOT and rebuild for full CEF host.");
  std::cout << "Run demo: 1) cef_host (with CEF)  2) cd demo/frontend && npm run dev" << std::endl;
  cef_api::APIInterface api;
  const std::string req = R"({"type":"call","callId":"t1","method":"readApp","params":{"id":1}})";
  std::cout << "API check: " << api.process(req).substr(0, 80) << "..." << std::endl;
  return 0;
#endif
}
