#pragma once

#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>

namespace cef_api::dto {

struct ApplicationData {
  int id{0};
  std::string name;
  std::string status;
};

void to_json(nlohmann::json& j, const ApplicationData& p);
void from_json(const nlohmann::json& j, ApplicationData& p);

}  // namespace cef_api::dto
