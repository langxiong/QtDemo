#pragma once

#include <Poco/AutoPtr.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/Util/IniFileConfiguration.h>

#include <string>

namespace common::config {

class Config {
public:
  Config() = default;

  /** Wrap an existing configuration (non-owning; used with Application::config()). */
  explicit Config(Poco::Util::AbstractConfiguration& cfg);

  bool load(const std::string& path);

  std::string getString(const std::string& key, const std::string& defaultValue) const;
  int getInt(const std::string& key, int defaultValue) const;
  double getDouble(const std::string& key, double defaultValue) const;
  bool getBool(const std::string& key, bool defaultValue) const;

private:
  const Poco::Util::AbstractConfiguration* active() const;

  Poco::AutoPtr<Poco::Util::IniFileConfiguration> _owned;
  /** Non-owning pointer; referenced config must outlive this Config instance. Used when wrapping Application::config(). */
  Poco::Util::AbstractConfiguration* _wrapped{nullptr};
};

} // namespace common::config
