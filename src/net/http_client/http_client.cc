// clang-format off
#include "http_client.h"
#include <list>
#include <sstream>
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Infos.hpp"
#include "curlpp/Options.hpp"
#include "src/common/assert.h"
#include "src/common/enum_name.hpp"
#include "src/common/except.h"
// clang-format on

namespace ohno {
namespace net {

/**
 * @brief 发起 HTTP 请求
 *
 * @param method 请求方法
 * @param uri URI
 * @param resp_body 响应体（返回值）
 * @param req_body 请求体（可以为空）
 * @param token token（可以为空）
 * @param ca_path CA 证书目录（可以为空）
 * @return HttpCode HTTP 响应码
 */
auto HttpClient::httpRequest(HttpMethod method, std::string_view uri, std::string &resp_body,
                             std::string_view req_body, std::string_view token,
                             std::string_view ca_path) const -> HttpCode {
  OHNO_ASSERT(!uri.empty());
  HttpCode code = HttpCode::Bad_Request;

  try {
    curlpp::Easy request{};
    std::ostringstream response{};

    std::list<std::string> headers{};
    if (!token.empty()) {
      headers.emplace_back(fmt::format("Authorization: Bearer {}", token));
    }
    headers.emplace_back("Content-Type: application/json");

    // request.setOpt(new curlpp::options::Verbose(true)); // TODO: 根据日志等级设置
    request.setOpt(new curlpp::options::Url(std::string{uri}));
    request.setOpt(curlpp::options::FollowLocation(true));
    request.setOpt(new curlpp::options::HttpHeader(headers));
    if (!ca_path.empty()) {
      request.setOpt(new curlpp::options::SslVerifyPeer(true));
      request.setOpt(new curlpp::options::CaInfo(std::string{ca_path}));
    } else {
      request.setOpt(new curlpp::options::SslVerifyPeer(false));
    }
    request.setOpt(new curlpp::options::WriteStream(&response));

    OHNO_LOG(trace, "HTTP {} request", enumName(method));
    switch (method) {
    case HttpMethod::GET:
    case HttpMethod::POST:
      break;
    case HttpMethod::DELETE:
      request.setOpt(new curlpp::options::CustomRequest("DELETE"));
      break;
    case HttpMethod::PATCH:
      request.setOpt(new curlpp::options::CustomRequest("PATCH"));
      break;
    default:
      throw OHNO_EXCEPT("Unsupported http method", false);
    }

    if (!req_body.empty()) {
      OHNO_LOG(trace, "HTTP request body:\n{}", req_body);
      request.setOpt(new curlpp::options::PostFields(std::string{req_body}));
      request.setOpt(new curlpp::options::PostFieldSize(static_cast<long>(req_body.size())));
    }

    request.perform();
    code = static_cast<HttpCode>(curlpp::infos::ResponseCode::get(request));
    resp_body = response.str();

    if (!resp_body.empty()) {
      OHNO_LOG(trace, "HTTP response body:\n{}", resp_body);
    }
  } catch (curlpp::LogicError &e) {
    resp_body = e.what();
    OHNO_LOG(warn, "HTTP req failed: {}", e.what());
  } catch (curlpp::RuntimeError &e) {
    resp_body = e.what();
    OHNO_LOG(warn, "HTTP req failed: {}", e.what());
  } catch (const std::exception &e) {
    resp_body = e.what();
    OHNO_LOG(warn, "HTTP req failed: {}", e.what());
  }

  return code;
}

} // namespace net
} // namespace ohno
