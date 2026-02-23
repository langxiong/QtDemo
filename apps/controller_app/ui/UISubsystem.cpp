#if defined(_WIN32) && defined(CONTROLLER_HAVE_DUILIB)

#include "UISubsystem.hpp"
#include "MainWindow.hpp"

namespace controller_app {

struct UISubsystem::Impl {
  std::unique_ptr<MainWindow> window;
};

UISubsystem::UISubsystem() : impl_(std::make_unique<Impl>()) {}

UISubsystem::~UISubsystem() = default;

bool UISubsystem::create(int width, int height, const wchar_t* title) {
  impl_->window = std::make_unique<MainWindow>(
    title ? std::wstring(title) : L"Medical Robot Control Demo",
    width, height);
  return impl_->window->create();
}

void UISubsystem::show() {
  if (impl_->window) impl_->window->show();
}

void UISubsystem::close() {
  if (impl_->window) impl_->window->close();
}

void* UISubsystem::getClientHwnd() const {
  return impl_->window ? impl_->window->getClientHwnd() : nullptr;
}

void UISubsystem::runMessageLoop() {
  if (impl_->window) impl_->window->run();
}

}  // namespace controller_app

#endif  // _WIN32 && CONTROLLER_HAVE_DUILIB
