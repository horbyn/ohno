// clang-format off
#include "vxlan.h"
#include "spdlog/fmt/fmt.h"
#include "src/cni/cni_config.h"
#include "src/common/assert.h"
#include "src/common/enum_name.hpp"
#include "src/common/except.h"
#include "src/net/fdb.h"
#include "src/net/macro.h"
#include "src/net/neigh.h"
#include "src/net/route.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 启动后端线程
 *
 * @param node_name 当前 Kubernetes 节点名称
 */
auto Vxlan::start(std::string_view node_name) -> void {
  Backend::startImpl(node_name, enumName(cni::CniConfigIpam::Mode::vxlan));
}

/**
 * @brief 设置持久化对象
 *
 * @param storage 持久化对象
 */
auto Vxlan::setStorage(std::unique_ptr<cni::StorageIf> storage) -> void {
  storage_ = std::move(storage);
}

/**
 * @brief 触发事件
 *
 * @param current_node 当前 Kubernetes 节点名称
 */
auto Vxlan::eventHandler(std::string_view current_node) -> void {
  OHNO_ASSERT(center_ != nullptr);
  OHNO_ASSERT(storage_ != nullptr);
  OHNO_ASSERT(nic_ != nullptr);

  // 集群是直接从 Kubernetes api server 中获取的，包含所有节点
  auto cluster = center_->getKubernetesData();

  // 持久化是从分布式缓存中获取的，仅包含创建了 VTEP 的节点
  std::string current_vtep_addr{}, current_vtep_mac{};
  storage_->getVtep(current_node, current_vtep_addr, current_vtep_mac);

  if (!current_vtep_addr.empty()) {
    // 检查节点新增
    for (const auto &[name, info] : cluster) {
      if (current_node == name) {
        continue; // 跳过自己
      }
      std::string vtep_addr{}, vtep_mac{};
      storage_->getVtep(name, vtep_addr, vtep_mac);

      // 出现在持久化中，并且之前没有缓存过
      if (!vtep_addr.empty() && node_cache_.find(name) == node_cache_.end()) {

        // 增加静态路由
        if (!nic_->addRoute(
                std::make_unique<net::Route>(info.pod_cidr_, vtep_addr, net::NAME_VXLAN),
                net::NetlinkIf::RouteNHFlags::onlink)) {
          OHNO_LOG(warn, "Vxlan failed to create static route(dest:{}, via:{}, dev:{})",
                   info.pod_cidr_, vtep_addr, net::NAME_VXLAN);
          continue;
        }

        // 增加 ARP 表项
        if (!nic_->addNeigh(std::make_unique<net::Neigh>(vtep_addr, vtep_mac, net::NAME_VXLAN))) {
          OHNO_LOG(warn, "Vxlan failed to create ARP entry(addr:{}, mac:{}) for {}", vtep_addr,
                   vtep_mac, net::NAME_VXLAN);
          nic_->delRoute(info.pod_cidr_, vtep_addr, std::string{});
          continue;
        }

        // 增加 FDB 表项
        if (!nic_->addFdb(
                std::make_unique<net::Fdb>(vtep_mac, info.internal_ip_, net::NAME_VXLAN))) {
          OHNO_LOG(warn, "Vxlan failed to create FDB entry(mac:{}, underlay:{})", vtep_mac,
                   info.internal_ip_);
          nic_->delRoute(info.pod_cidr_, vtep_addr, std::string{});
          nic_->delNeigh(vtep_addr, vtep_mac, net::NAME_VXLAN);
          continue;
        }

        node_cache_.emplace(name, info);
        OHNO_LOG(info,
                 "Vxlan static route(dest:{}, via:{}), ARP cache(addr:{}, mac:{}), FDB(mac:{}, "
                 "underlay:{}) existed",
                 info.pod_cidr_, vtep_addr, vtep_addr, vtep_mac, vtep_mac, info.internal_ip_);
      }
    } // end for(cluster)
  }

  // 检查节点删除
  for (auto it = node_cache_.begin(); it != node_cache_.end();) {
    const auto &[name, info] = *it;

    // 只有两种情况：
    // 1. 当前节点已被删除，则节点内所有静态路由、ARP 缓存、FDB 表项都要删掉
    // 2. 其他节点被删除了，则节点只需要删除对应的静态路由、ARP 缓存、FDB 表项
    std::string vtep_addr{}, vtep_mac{};
    storage_->getVtep(name, vtep_addr, vtep_mac);
    if (current_vtep_addr.empty() || vtep_addr.empty()) {
      if (!nic_->delRoute(info.pod_cidr_, vtep_addr, net::NAME_VXLAN)) {
        OHNO_LOG(warn, "Vxlan failed to delete static route(dest:{}, via:{}, dev:{})",
                 info.pod_cidr_, vtep_addr, net::NAME_VXLAN);
      }

      if (!nic_->delNeigh(vtep_addr, vtep_mac, net::NAME_VXLAN)) {
        OHNO_LOG(warn, "Vxlan failed to delete ARP cache(addr:{}, mac:{}) for {}", vtep_addr,
                 vtep_mac, net::NAME_VXLAN);
      }

      if (!nic_->delFdb(info.internal_ip_, vtep_mac, net::NAME_VXLAN)) {
        OHNO_LOG(warn, "Vxlan failed to delete FDB(mac:{}, underlay:{})", vtep_mac,
                 info.internal_ip_);
      }

      it = node_cache_.erase(it);
      OHNO_LOG(info,
               "Vxlan static route(dest:{}, via:{}), ARP cache(addr:{}, mac:{}), FDB(mac:{}, "
               "underlay:{}) has been erased",
               info.pod_cidr_, vtep_addr, vtep_addr, vtep_mac, vtep_mac, info.internal_ip_);
      continue;
    } // end if()
    ++it;
  } // end for()
}

} // namespace backend
} // namespace ohno
