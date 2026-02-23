// Windows entry point for cef_host (CEF mode).
// Uses wWinMain for WIN32 subsystem; CEF runs in main process.
// Integrates Poco::Util::Application for --parent-hwnd parsing.

#include "CefHostApplication.hpp"
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

static std::string wideToUtf8(const wchar_t* wstr) {
  if (!wstr || !*wstr) return {};
  int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
  if (len <= 0) return {};
  std::string result(static_cast<size_t>(len), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], len, nullptr, nullptr);
  result.resize(result.find('\0'));
  return result;
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine,
                     int nCmdShow) {
  (void)hInstance;
  (void)hPrevInstance;
  (void)lpCmdLine;
  (void)nCmdShow;

  int argc = 0;
  LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
  std::vector<std::string> argStrings;
  if (wargv && argc > 0) {
    argStrings.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
      argStrings.push_back(wideToUtf8(wargv[i]));
    }
    LocalFree(wargv);
  } else {
    argStrings.push_back("cef_host");
    argc = 1;
  }

  std::vector<char*> argvPtrs;
  argvPtrs.reserve(static_cast<size_t>(argc) + 1);
  for (auto& s : argStrings) {
    argvPtrs.push_back(&s[0]);
  }
  argvPtrs.push_back(nullptr);

  CefHostApplication app;
  app.init(argc, argvPtrs.data());
  return app.run(argc, argvPtrs.data());
}
