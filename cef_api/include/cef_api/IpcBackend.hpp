#pragma once

#include "cef_api/APIInterface.hpp"
#include <memory>
#include <string>

namespace Poco {
namespace Net {
class StreamSocket;
}
}  // namespace Poco

namespace cef_api {

/** IPC backend: forwards API requests to controller_app via TCP. */
class IpcBackend {
public:
  /** Create backend connecting to host:port. Returns nullptr if connection fails. */
  static std::unique_ptr<IpcBackend> create(const std::string& host, int port);

  ~IpcBackend();

  /** Forward request to controller_app. Returns response or empty on error. */
  std::string process(const std::string& requestJson);

private:
  explicit IpcBackend(std::unique_ptr<Poco::Net::StreamSocket> socket);

  std::unique_ptr<Poco::Net::StreamSocket> _socket;
};

}  // namespace cef_api
