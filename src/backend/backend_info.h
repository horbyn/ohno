#pragma once

// clang-format off
#include <string>
// clang-format on

namespace ohno {
namespace backend {

constexpr std::string_view PATH_CNI_CONF{"/etc/cni/net.d/ohno.json"};

struct BackendInfo {
  std::string api_server_; // Kubernetes api server
  bool ssl_;               // 是否检查证书（true）
  int refresh_interval_;   // daemon 向 api server 发起请求的间隔，单位秒
};

} // namespace backend
} // namespace ohno
