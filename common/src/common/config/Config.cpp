#include "common/config/Config.h"

#include <Poco/Util/AbstractConfiguration.h>

namespace common::config {

bool Config::load(const std::string& path) {
  _config = new Poco::Util::IniFileConfiguration(path);
  return _config != nullptr;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
  if (!_config) return defaultValue;
  return _config->getString(key, defaultValue);
}

int Config::getInt(const std::string& key, int defaultValue) const {
  if (!_config) return defaultValue;
  return _config->getInt(key, defaultValue);
}

double Config::getDouble(const std::string& key, double defaultValue) const {
  if (!_config) return defaultValue;
  return _config->getDouble(key, defaultValue);
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
  if (!_config) return defaultValue;
  return _config->getBool(key, defaultValue);
}

} // namespace common::config
