#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace cef_api::dto {

/** Request envelope for notify/call protocol. */
struct ApiRequest {
  std::string type;   // "notify" or "call"
  std::string method; // "readApp", "writeApp", "readStream", "writeStream"
  nlohmann::json params;
  std::optional<std::string> callId;  // required for type "call"
};

/** Call response envelope. Success and error share this struct; result has error_code:
 *  error_code=0 success, error_code!=0 error (with error message). JS parses. */
struct ApiCallResponse {
  std::string callId;
  nlohmann::json result;
};

void to_json(nlohmann::json& j, const ApiCallResponse& p);

}  // namespace cef_api::dto
