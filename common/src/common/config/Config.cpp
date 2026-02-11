#include "common/config/Config.h"

#include <Poco/Util/AbstractConfiguration.h>

namespace common::config {

Config::Config(Poco::Util::AbstractConfiguration& cfg) : _wrapped(&cfg) {}

bool Config::load(const std::string& path) {
  _owned = new Poco::Util::IniFileConfiguration(path);
  _wrapped = nullptr;
  return _owned != nullptr;
}

const Poco::Util::AbstractConfiguration* Config::active() const {
  if (_wrapped) return _wrapped;
  return _owned.get();
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
  auto* c = active();
  if (!c) return defaultValue;
  return c->getString(key, defaultValue);
}

int Config::getInt(const std::string& key, int defaultValue) const {
  auto* c = active();
  if (!c) return defaultValue;
  return c->getInt(key, defaultValue);
}

double Config::getDouble(const std::string& key, double defaultValue) const {
  auto* c = active();
  if (!c) return defaultValue;
  return c->getDouble(key, defaultValue);
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
  auto* c = active();
  if (!c) return defaultValue;
  return c->getBool(key, defaultValue);
}

} // namespace common::config
