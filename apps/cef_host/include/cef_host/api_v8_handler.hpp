#pragma once

#include <include/cef_v8.h>
#include <string>

namespace cef_host {

/** V8 handler for window.api.notify and window.api.call. */
class ApiV8Handler : public CefV8Handler {
public:
  ApiV8Handler() = default;
  bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments,
               CefRefPtr<CefV8Value>& retval, CefString& exception) override;

private:
  IMPLEMENT_REFCOUNTING(ApiV8Handler);
};

}  // namespace cef_host
