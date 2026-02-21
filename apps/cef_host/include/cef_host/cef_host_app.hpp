#pragma once

#include "cef_api/APIInterface.hpp"
#include "cef_host/cef_render_handler.hpp"
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_life_span_handler.h>
#include <memory>

namespace cef_host {

/** CEF app: owns APIInterface, creates browser window, handles process messages. */
class CefHostApp : public CefApp, public CefBrowserProcessHandler, public CefClient, public CefLifeSpanHandler {
public:
  explicit CefHostApp(std::shared_ptr<cef_api::APIInterface> api);
  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return render_handler_; }
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  void OnContextInitialized() override;
  bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                               CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  bool DoClose(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

private:
  std::shared_ptr<cef_api::APIInterface> api_;
  CefRefPtr<CefBrowser> browser_;
  CefRefPtr<CefRenderHandler> render_handler_{new CefRenderHandler};
  IMPLEMENT_REFCOUNTING(CefHostApp);
};

}  // namespace cef_host
