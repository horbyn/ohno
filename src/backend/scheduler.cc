// clang-format off
#include "scheduler.h"
#include "src/common/assert.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 设置后端策略
 *
 * @param backend 后端对象
 */
auto Scheduler::setStrategy(std::unique_ptr<BackendIf> backend) -> void {
  backend_ = std::move(backend);
}

/**
 * @brief 执行后端策略
 *
 * @param node_name 当前 Kubernetes 节点名称
 */
auto Scheduler::start(std::string_view node_name) -> void {
  OHNO_ASSERT(backend_);
  backend_->start(node_name);
}

/**
 * @brief 结束后端策略
 *
 */
auto Scheduler::stop() -> void {
  if (backend_) {
    backend_->stop();
  }
}

} // namespace backend
} // namespace ohno
