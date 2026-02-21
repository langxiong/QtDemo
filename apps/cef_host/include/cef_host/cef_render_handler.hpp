#pragma once

#include <include/cef_app.h>
#include <include/cef_render_process_handler.h>

namespace cef_host {

/** Render process handler: injects window.api via OnContextCreated, handles response messages. */
class CefRenderHandler : public CefRenderProcessHandler {
public:
  CefRenderHandler() = default;
  void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefV8Context> context) override;
  bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                               CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

private:
  IMPLEMENT_REFCOUNTING(CefRenderHandler);
};

}  // namespace cef_host
