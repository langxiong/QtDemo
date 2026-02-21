#include "cef_api/dto/ApiEnvelope.hpp"

namespace cef_api::dto {

void to_json(nlohmann::json& j, const ApiCallResponse& p) {
  j = nlohmann::json{
    {"callId", p.callId},
    {"result", p.result},
  };
}

}  // namespace cef_api::dto
