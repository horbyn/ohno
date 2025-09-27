// clang-format off
#include "scheduler.h"
#include <fstream>
#include "spdlog/fmt/fmt.h"
#include "src/common/assert.h"
#include "src/cni/cni_config.h"
#include "src/common/except.h"
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
 * @param netlink Netlink 对象
 * @param backend_info 后端信息
 */
auto Scheduler::start(std::string_view node_name, std::weak_ptr<ohno::net::NetlinkIf> netlink,
                      const ohno::backend::BackendInfo &backend_info) -> void {
  OHNO_ASSERT(backend_);

  std::ifstream ifile{std::string{PATH_CNI_CONF}};
  if (!ifile.is_open()) {
    throw OHNO_EXCEPT(fmt::format("Cannot open CNI configuration:{}", PATH_CNI_CONF), false);
  }

  nlohmann::json json{};
  ifile >> json;
  cni::CniConfig cni_conf = json;
  if (cni_conf.ipam_.mode_ == cni::CniConfigIpam::Mode::RESERVED) {
    throw OHNO_EXCEPT("CNI configuration has invalid entry(ipam.mode)", false);
  }

  backend_->start(cni_conf.ipam_.mode_, node_name, netlink, backend_info);
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
