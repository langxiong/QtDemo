#pragma once

#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)

#include <memory>
#include <string>

namespace controller_app {

/** DuiLib main window; provides HWND for CEF embed (client area parent). */
class MainWindow {
public:
  explicit MainWindow(const std::wstring& title, int width, int height);
  ~MainWindow();

  bool create();
  void show();
  void close();

  /** Parent HWND for CEF SetAsChild (main window encompasses client area). */
  void* getClientHwnd() const;

  /** Run message loop; returns when window closes. */
  int run();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace controller_app

#endif  // _WIN32 && CONTROLLER_HAVE_DUILIB
