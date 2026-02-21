#pragma once

#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>

namespace cef_api::dto {

struct StreamData {
  int streamId{0};
  std::string payload;
  std::int64_t timestamp{0};
};

void to_json(nlohmann::json& j, const StreamData& p);
void from_json(const nlohmann::json& j, StreamData& p);

}  // namespace cef_api::dto
