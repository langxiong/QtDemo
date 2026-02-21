#include "cef_host/api_v8_handler.hpp"
#include "cef_host/cef_render_handler.hpp"
#include "common/log/Log.h"
#include <include/cef_process_message.h>

namespace cef_host {

void CefRenderHandler::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                        CefRefPtr<CefV8Context> context) {
  CefRefPtr<CefV8Value> global = context->GetGlobal();
  CefRefPtr<CefV8Value> api = CefV8Value::CreateObject(nullptr, nullptr);

  CefRefPtr<ApiV8Handler> handler = new ApiV8Handler();
  api->SetValue("notify", CefV8Value::CreateFunction("notify", handler), V8_PROPERTY_ATTRIBUTE_NONE);
  api->SetValue("call", CefV8Value::CreateFunction("call", handler), V8_PROPERTY_ATTRIBUTE_NONE);

  global->SetValue("api", api, V8_PROPERTY_ATTRIBUTE_READONLY);
  common::log::Info("cef_host", "window.api injected");
}

bool CefRenderHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                 CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
  if (message->GetName().ToString() != "cef_api_response") return false;

  CefRefPtr<CefListValue> args = message->GetArgumentList();
  if (args->GetSize() < 2) return false;

  std::string callId = args->GetString(0).ToString();
  std::string resultJson = args->GetString(1).ToString();
  std::string escaped;
  for (char c : resultJson) {
    if (c == '\\') escaped += "\\\\";
    else if (c == '\'') escaped += "\\'";
    else if (c == '\n') escaped += "\\n";
    else if (c == '\r') escaped += "\\r";
    else escaped += c;
  }

  std::string code =
      "(function(){"
      "var c=window.__cefApiCallbacks&&window.__cefApiCallbacks['" + callId + "'];"
      "if(c){delete window.__cefApiCallbacks['" + callId + "'];"
      "try{var r=JSON.parse('" + escaped + "');c[0](r);}catch(e){c[1](e);}}"
      "})()";

  frame->ExecuteJavaScript(code, frame->GetURL(), 0);
  return true;
}

}  // namespace cef_host
