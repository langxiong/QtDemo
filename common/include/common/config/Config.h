#pragma once

#include <Poco/AutoPtr.h>
#include <Poco/Util/IniFileConfiguration.h>

#include <string>

namespace common::config {

class Config {
public:
  bool load(const std::string& path);

  std::string getString(const std::string& key, const std::string& defaultValue) const;
  int getInt(const std::string& key, int defaultValue) const;
  double getDouble(const std::string& key, double defaultValue) const;
  bool getBool(const std::string& key, bool defaultValue) const;

private:
  Poco::AutoPtr<Poco::Util::IniFileConfiguration> _config;
};

} // namespace common::config
