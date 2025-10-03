#pragma once

// clang-format off
#include <memory>
#include "backend_if.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 用策略模式来组织路由的维护功能：
 * 如静态路由、动态路由或隧道策略，调度器对象作为策略的 context
 *
 */
class SchedulerIf {
public:
  virtual ~SchedulerIf() = default;
  virtual auto setStrategy(std::unique_ptr<BackendIf> backend) -> void = 0;
  virtual auto start(std::string_view node_name) -> void = 0;
  virtual auto stop() -> void = 0;
};

} // namespace backend
} // namespace ohno
