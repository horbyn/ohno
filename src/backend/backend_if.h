#pragma once

// clang-format off
#include <memory>
#include <string_view>
#include "src/backend/center_if.h"
#include "src/net/nic_if.h"
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
  virtual auto start(std::string_view node_name) -> void = 0;
  virtual auto setInterval(int sec) -> void = 0;
  virtual auto setCenter(std::unique_ptr<backend::CenterIf> center) -> void = 0;
  virtual auto setNic(std::unique_ptr<net::NicIf> nic) -> void = 0;
  virtual auto stop() -> void = 0;

protected:
  int interval_;
  std::unique_ptr<backend::CenterIf> center_;
  std::unique_ptr<net::NicIf> nic_;
};

} // namespace backend
} // namespace ohno
