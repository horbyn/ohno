#pragma once

// clang-format off
#include "src/log/logger.h"
#include "http_client_if.h"
// clang-format on

namespace ohno {
namespace net {

class HttpClient : public HttpClientIf, public log::Loggable<log::Id::net> {
public:
  auto httpRequest(HttpMethod method, std::string_view uri, std::string &resp_body,
                   std::string_view req_body, std::string_view token = {},
                   std::string_view ca_path = {}) const -> HttpCode override;
};

} // namespace net
} // namespace ohno
