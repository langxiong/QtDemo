#include "common/config/Config.h"

#include <Poco/AutoPtr.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/Util/IniFileConfiguration.h>

#include <memory>

namespace common::config {

struct Config::ConfigImpl {
  Poco::AutoPtr<Poco::Util::IniFileConfiguration> owned;
  Poco::Util::AbstractConfiguration* wrapped{nullptr};

  const Poco::Util::AbstractConfiguration* active() const {
    if (wrapped) return wrapped;
    return owned.get();
  }
};

Config::Config() : _impl(std::make_unique<ConfigImpl>()) {}

Config::Config(std::unique_ptr<ConfigImpl> impl) : _impl(std::move(impl)) {}

Config::~Config() = default;

bool Config::load(const std::string& path) {
  _impl->owned = new Poco::Util::IniFileConfiguration(path);
  _impl->wrapped = nullptr;
  return _impl->owned != nullptr;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
  auto* c = _impl->active();
  if (!c) return defaultValue;
  return c->getString(key, defaultValue);
}

int Config::getInt(const std::string& key, int defaultValue) const {
  auto* c = _impl->active();
  if (!c) return defaultValue;
  return c->getInt(key, defaultValue);
}

double Config::getDouble(const std::string& key, double defaultValue) const {
  auto* c = _impl->active();
  if (!c) return defaultValue;
  return c->getDouble(key, defaultValue);
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
  auto* c = _impl->active();
  if (!c) return defaultValue;
  return c->getBool(key, defaultValue);
}

Config Config::WrapPocoConfig(void* pocoAbstractConfiguration) {
  auto* ac = static_cast<Poco::Util::AbstractConfiguration*>(pocoAbstractConfiguration);
  auto impl = std::make_unique<ConfigImpl>();
  impl->wrapped = ac;
  return Config(std::move(impl));
}

} // namespace common::config
