#include "cef_host/api_v8_handler.hpp"
#include <include/cef_process_message.h>
#include <chrono>

namespace cef_host {

static std::string V8ObjectToJson(CefRefPtr<CefV8Value> v) {
  if (!v->IsObject()) return "{}";
  CefRefPtr<CefV8Value> json = CefV8Context::GetCurrentContext()->GetGlobal()->GetValue("JSON");
  if (json && json->IsObject()) {
    CefRefPtr<CefV8Value> stringify = json->GetValue("stringify");
    if (stringify->IsFunction()) {
      CefV8ValueList args;
      args.push_back(v);
      CefRefPtr<CefV8Value> ret = stringify->ExecuteFunction(v, args);
      if (ret && ret->IsString()) return ret->GetStringValue().ToString();
    }
  }
  return "{}";
}

bool ApiV8Handler::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments,
                            CefRefPtr<CefV8Value>& retval, CefString& exception) {
  if (arguments.size() < 2) {
    exception = "api." + name.ToString() + "(method, params) requires 2 arguments";
    return false;
  }
  if (!arguments[0]->IsString()) {
    exception = "api." + name.ToString() + ": method must be string";
    return false;
  }

  const CefString method = arguments[0]->GetStringValue();
  std::string paramsJson = arguments.size() >= 2 ? V8ObjectToJson(arguments[1]) : "{}";

  CefRefPtr<CefFrame> frame = CefV8Context::GetCurrentContext()->GetFrame();
  if (!frame) {
    exception = "No frame";
    return false;
  }

  CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("cef_api_request");
  CefRefPtr<CefListValue> args = msg->GetArgumentList();
  args->SetString(0, name);
  args->SetString(1, method);
  args->SetString(2, paramsJson);

  if (name == "call") {
    static int callIdGen = 0;
    auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string callId = "cef-" + std::to_string(++callIdGen) + "-" + std::to_string(ts);
    args->SetString(3, callId);

    CefRefPtr<CefV8Context> ctx = CefV8Context::GetCurrentContext();
    CefRefPtr<CefV8Value> global = ctx->GetGlobal();
    CefRefPtr<CefV8Value> promiseCtor = global->GetValue("Promise");
    if (!promiseCtor || !promiseCtor->IsFunction()) {
      exception = "Promise not available";
      return false;
    }

    std::string code =
        "(function(id){"
        "if(!window.__cefApiCallbacks)window.__cefApiCallbacks={};"
        "return new Promise(function(resolve,reject){window.__cefApiCallbacks[id]=[resolve,reject];});"
        "})('" + callId + "')";

    CefRefPtr<CefV8Value> promise;
    CefRefPtr<CefV8Exception> ex;
    if (!ctx->Eval(code, CefString(), 0, promise, ex)) {
      exception = ex ? ex->GetMessage().ToString() : "Promise creation failed";
      return false;
    }
    frame->SendProcessMessage(PID_BROWSER, msg);
    retval = promise;
  } else {
    frame->SendProcessMessage(PID_BROWSER, msg);
    retval = CefV8Value::CreateUndefined();
  }
  return true;
}

}  // namespace cef_host
