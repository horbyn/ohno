#pragma once

// clang-format off
#include <functional>
#include "backend_info.h"
#include "src/cni/cni_config.h"
#include "src/net/netlink/netlink_if.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 用策略模式来组织路由的维护功能：
 * 如静态路由、动态路由或隧道策略，后端对象作为策略
 *
 */
class BackendIf {
public:
  virtual ~BackendIf() = default;
  virtual auto start(cni::CniConfigIpam::Mode mode, std::string_view node_name,
                     std::weak_ptr<net::NetlinkIf> netlink, const BackendInfo &backend_info)
      -> void = 0;
  virtual auto stop() -> void = 0;
};

} // namespace backend
} // namespace ohno
