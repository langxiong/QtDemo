#include "cef_host/cef_host_app.hpp"
#include "common/log/Log.h"
#include <include/cef_browser.h>
#include <include/cef_command_line.h>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace cef_host {

CefHostApp::CefHostApp(std::shared_ptr<cef_api::APIInterface> api) : api_(std::move(api)) {}

void CefHostApp::OnContextInitialized() {
#if defined(_WIN32)
  CefWindowInfo window_info;
  window_info.SetAsPopup(nullptr, "CEF React Demo");

  CefBrowserSettings settings;
  CefString url = "http://localhost:5173";  // Dev: npm run dev. Prod: file:// to frontend/dist/index.html

  CefBrowserHost::CreateBrowser(window_info, this, url, settings, nullptr, nullptr);
#else
  (void)0;
#endif
}

bool CefHostApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                          CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
  if (message->GetName().ToString() != "cef_api_request") return false;

  CefRefPtr<CefListValue> args = message->GetArgumentList();
  if (args->GetSize() < 3) return false;

  std::string type = args->GetString(0).ToString();
  std::string method = args->GetString(1).ToString();
  std::string paramsJson = args->GetSize() > 2 ? args->GetString(2).ToString() : "{}";
  std::string callId = args->GetSize() > 3 ? args->GetString(3).ToString() : "";

  nlohmann::json req;
  req["type"] = type;
  req["method"] = method;
  req["params"] = nlohmann::json::parse(paramsJson.empty() ? "{}" : paramsJson);
  if (!callId.empty()) req["callId"] = callId;

  std::string resp = api_->process(req.dump());

  if (type == "call" && !resp.empty()) {
    CefRefPtr<CefProcessMessage> respMsg = CefProcessMessage::Create("cef_api_response");
    CefRefPtr<CefListValue> respArgs = respMsg->GetArgumentList();
    respArgs->SetString(0, callId);
    respArgs->SetString(1, resp);
    frame->SendProcessMessage(PID_RENDERER, respMsg);
  }
  return true;
}

void CefHostApp::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  browser_ = browser;
}

bool CefHostApp::DoClose(CefRefPtr<CefBrowser> browser) {
  return false;
}

void CefHostApp::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  browser_ = nullptr;
}

}  // namespace cef_host
