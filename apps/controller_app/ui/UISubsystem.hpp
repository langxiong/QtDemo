#pragma once

#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)

#include <memory>

namespace controller_app {

/** UISubsystem: owns DuiLib MainWindow, provides client HWND for cef_host. */
class UISubsystem {
public:
  UISubsystem();
  ~UISubsystem();

  bool create(int width, int height, const wchar_t* title);
  void show();
  void close();

  /** Parent HWND for CEF SetAsChild (main window client area). */
  void* getClientHwnd() const;

  /** Run message loop; returns when window closes. */
  void runMessageLoop();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace controller_app

#endif  // _WIN32 && CONTROLLER_HAVE_DUILIB
