#include "cef_api/dto/StreamData.hpp"
#include <nlohmann/json.hpp>

namespace cef_api::dto {

void to_json(nlohmann::json& j, const StreamData& p) {
  j = nlohmann::json{
    {"streamId", p.streamId},
    {"payload", p.payload},
    {"timestamp", p.timestamp},
  };
}

void from_json(const nlohmann::json& j, StreamData& p) {
  j.at("streamId").get_to(p.streamId);
  j.at("payload").get_to(p.payload);
  j.at("timestamp").get_to(p.timestamp);
}

}  // namespace cef_api::dto
