#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)

#include "MainWindow.hpp"
#include "common/log/Log.h"
#include <duilib/UIlib.h>
#include <windows.h>
#include <memory>
#include <string>

namespace controller_app {

using namespace DuiLib;

class MainWindowImpl : public WindowImplBase {
public:
  MainWindowImpl(const std::wstring& title, int width, int height)
    : title_(title), width_(width), height_(height) {}

  CDuiString GetSkinFile() override {
    return _T("mainwindow.xml");
  }

  LPCTSTR GetWindowClassName() const override {
    return _T("ControllerMainWindow");
  }

  UINT GetClassStyle() const override {
    return CS_DBLCLKS;
  }

  void InitResource() override {
    wchar_t exePath[MAX_PATH] = {0};
    if (::GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
      std::wstring path(exePath);
      size_t sep = path.find_last_of(L"\\/");
      if (sep != std::wstring::npos) path.resize(sep + 1);
      m_pm.SetResourcePath(path.c_str());
    }
  }

  LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override {
    if (uMsg == WM_DESTROY) {
      PostQuitMessage(0);
      bHandled = TRUE;
      return 0;
    }
    return __super::HandleCustomMessage(uMsg, wParam, lParam, bHandled);
  }

  /** Run DuiLib message loop (uses GetMessage, returns when WM_QUIT). */
  void runMessageLoop() {
    CPaintManagerUI::MessageLoop();
  }

  std::wstring title_;
  int width_;
  int height_;
};

struct MainWindow::Impl {
  std::unique_ptr<MainWindowImpl> window;
  std::wstring title;
  int width;
  int height;
};

MainWindow::MainWindow(const std::wstring& title, int width, int height)
  : impl_(std::make_unique<Impl>()) {
  impl_->title = title;
  impl_->width = width;
  impl_->height = height;
}

MainWindow::~MainWindow() = default;

bool MainWindow::create() {
  impl_->window = std::make_unique<MainWindowImpl>(impl_->title, impl_->width, impl_->height);
  CDuiRect rc(0, 0, impl_->width, impl_->height);
  HWND parent = nullptr;
  DWORD style = WS_OVERLAPPEDWINDOW;
  DWORD exstyle = 0;
  if (!impl_->window->Create(parent, impl_->title.c_str(), style, exstyle, rc)) {
    common::log::Error("controller_app", "MainWindow Create failed");
    return false;
  }
  return true;
}

void MainWindow::show() {
  if (impl_->window && impl_->window->GetHWND()) {
    ShowWindow(impl_->window->GetHWND(), SW_SHOW);
  }
}

void MainWindow::close() {
  if (impl_->window && impl_->window->GetHWND()) {
    PostMessage(impl_->window->GetHWND(), WM_CLOSE, 0, 0);
  }
}

void* MainWindow::getClientHwnd() const {
  return impl_->window ? impl_->window->GetHWND() : nullptr;
}

int MainWindow::run() {
  if (!impl_->window) return -1;
  impl_->window->runMessageLoop();
  return 0;
}

}  // namespace controller_app

#endif  // _WIN32 && CONTROLLER_HAVE_DUILIB
