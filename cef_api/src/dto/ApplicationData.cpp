#include "cef_api/dto/ApplicationData.hpp"
#include <nlohmann/json.hpp>

namespace cef_api::dto {

void to_json(nlohmann::json& j, const ApplicationData& p) {
  j = nlohmann::json{
    {"id", p.id},
    {"name", p.name},
    {"status", p.status},
  };
}

void from_json(const nlohmann::json& j, ApplicationData& p) {
  j.at("id").get_to(p.id);
  j.at("name").get_to(p.name);
  j.at("status").get_to(p.status);
}

}  // namespace cef_api::dto
