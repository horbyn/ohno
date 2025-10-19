// clang-format off
#include "host_gw.h"
#include "spdlog/fmt/fmt.h"
#include "src/cni/cni_config.h"
#include "src/common/assert.h"
#include "src/common/enum_name.hpp"
#include "src/common/except.h"
#include "src/net/addr.h"
#include "src/net/route.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 启动后端线程
 *
 * @param node_name 当前 Kubernetes 节点名称
 */
auto HostGw::start(std::string_view node_name) -> void {
  Backend::startImpl(node_name, enumName(cni::CniConfigIpam::Mode::host_gw));
}

/**
 * @brief 设置持久化对象
 *
 * @param ipam 持久化对象
 */
auto HostGw::setIpam(std::unique_ptr<ipam::IpamIf> ipam) -> void { ipam_ = std::move(ipam); }

/**
 * @brief 触发事件
 *
 * @param current_node 当前 Kubernetes 节点名称
 */
auto HostGw::eventHandler(std::string_view current_node) -> void {
  OHNO_ASSERT(center_ != nullptr);
  OHNO_ASSERT(ipam_ != nullptr);
  OHNO_ASSERT(nic_ != nullptr);

  // 集群是直接从 Kubernetes api server 中获取的，包含所有节点
  auto cluster = center_->getKubernetesData();

  // 持久化是从分布式缓存中获取的，仅包含创建了 CNI 插件的节点
  std::string subnet_cur{};
  std::string subnet{};
  ipam_->getSubnet(current_node, subnet_cur);

  if (!subnet_cur.empty()) {
    // 检查节点新增
    for (const auto &[name, info] : cluster) {
      if (current_node == name) {
        continue; // 跳过自己
      }
      ipam_->getSubnet(name, subnet);

      // 出现在持久化中，并且之前没有缓存过
      if (!subnet.empty() && node_cache_.find(name) == node_cache_.end()) {
        if (!nic_->addRoute(
                std::make_unique<net::Route>(info.pod_cidr_, info.internal_ip_, std::string{}),
                net::NetlinkIf::RouteNHFlags::NONE)) {
          OHNO_LOG(warn, "Host-gw mode failed to create static route(dest:{}, via:{})",
                   info.pod_cidr_, info.internal_ip_);
        } else {
          node_cache_.emplace(name, info);
          OHNO_LOG(info, "Host-gw static route(dest:{}, via:{}) existed", info.pod_cidr_,
                   info.internal_ip_);
        }
      }
    }
  }

  // 检查节点删除
  for (auto it = node_cache_.begin(); it != node_cache_.end();) {
    const auto &[name, info] = *it;

    // 只有两种情况：
    // 1. 当前节点已被删除，则节点内所有静态路由都要删除
    // 2. 其他节点被删除了，则节点只需要删除对应的静态路由
    ipam_->getSubnet(name, subnet);
    if (subnet_cur.empty() || subnet.empty()) {
      if (!nic_->delRoute(info.pod_cidr_, info.internal_ip_, {})) {
        OHNO_LOG(warn, "Host-gw mode failed to delete static route(dest:{}, via:{})",
                 info.pod_cidr_, info.internal_ip_);
      } else {
        it = node_cache_.erase(it);
        OHNO_LOG(info, "Host-gw static route(dest:{}, via:{}) has been erased", info.pod_cidr_,
                 info.internal_ip_);
        continue; // 下一轮迭代直接使用当前迭代器
      }
    }

    ++it;
  }
}

} // namespace backend
} // namespace ohno
