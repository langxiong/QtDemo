// Windows entry point for cef_host (CEF mode).
// Uses wWinMain for WIN32 subsystem; CEF runs in main process.

#include "cef_host/cef_host_app.hpp"
#include "common/log/Log.h"
#include <include/cef_app.h>
#include <include/cef_command_line.h>
#include <include/cef_sandbox_win.h>
#include <windows.h>

int RunMain(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow, void* sandbox_info) {
  (void)lpCmdLine;
  (void)nCmdShow;

  common::log::Init("cef_host");

  CefMainArgs main_args(hInstance);

  CefRefPtr<cef_host::CefHostApp> app(new cef_host::CefHostApp(std::make_shared<cef_api::APIInterface>()));
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
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine,
                     int nCmdShow) {
  (void)hPrevInstance;

  void* sandbox_info = nullptr;
  return RunMain(hInstance, lpCmdLine, nCmdShow, sandbox_info);
}
