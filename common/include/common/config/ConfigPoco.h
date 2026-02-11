#pragma once

#include "common/config/Config.h"

#include <Poco/Util/AbstractConfiguration.h>

namespace common::config {

/** Wrap Poco Application::config() into Config. Use when passing config to InitFromConfig or FaultInjector::LoadFromConfig. */
inline Config WrapPocoConfig(Poco::Util::AbstractConfiguration& cfg) {
  return Config::WrapPocoConfig(static_cast<void*>(&cfg));
}

} // namespace common::config
