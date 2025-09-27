#pragma once

// clang-format off
#include <string>
#include <string_view>
// clang-format on

namespace ohno {
namespace net {

enum class HttpMethod : uint8_t { GET, POST, DELETE, PATCH };
enum class HttpCode {
  Ok = 200,
  Created = 201,
  Accepted = 202,
  No_Content = 204,
  Bad_Request = 400,
  Unauthorized = 401,
  Forbidden = 403,
  Not_Found = 404,
  Bad_Gateway = 502
};

class HttpClientIf {
public:
  virtual ~HttpClientIf() = default;
  virtual auto httpRequest(HttpMethod method, std::string_view uri, std::string &resp_body,
                           std::string_view req_body, std::string_view token = {},
                           std::string_view ca_path = {}) const -> HttpCode = 0;
};

} // namespace net
} // namespace ohno
