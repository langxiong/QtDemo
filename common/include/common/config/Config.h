#pragma once

#include <memory>
#include <string>

namespace common::config {

class Config {
public:
  Config();

  ~Config();

  bool load(const std::string& path);

  std::string getString(const std::string& key, const std::string& defaultValue) const;
  int getInt(const std::string& key, int defaultValue) const;
  double getDouble(const std::string& key, double defaultValue) const;
  bool getBool(const std::string& key, bool defaultValue) const;

  /** Wrap an opaque Poco config pointer. Prefer ConfigPoco.h WrapPocoConfig(AbstractConfiguration&) for type safety. */
  static Config WrapPocoConfig(void* pocoAbstractConfiguration);

private:
  struct ConfigImpl;
  std::unique_ptr<ConfigImpl> _impl;

  explicit Config(std::unique_ptr<ConfigImpl> impl);
};

} // namespace common::config
