// clang-format off
#include "ipam.h"
#include "spdlog/fmt/fmt.h"
#include "src/backend/center_if.h"
#include "src/common/assert.h"
#include "src/common/except.h"
#include "src/etcd/etcd_client_shell.h"
#include "src/net/subnet.h"
// clang-format on

namespace ohno {
namespace ipam {

/**
 * @brief IPAM 初始化
 *
 * @param etcd_client Etcd 客户端
 * @return true 初始化成功
 * @return false 初始化失败
 */
auto Ipam::init(std::unique_ptr<etcd::EtcdClientIf> etcd_client) -> bool {
  etcd_client_ = std::move(etcd_client);
  if (etcd_client_ == nullptr || !etcd_client_->test()) {
    OHNO_LOG(warn, "ETCD client initialization failed");
    return false;
  }
  return true;
}

/**
 * @brief 将 IPAM 信息全部输出出来
 *
 * @return std::string IPAM 结果，无结果为空
 */
auto Ipam::dump() const -> std::string {
  OHNO_ASSERT(etcd_client_);
  return etcd_client_->dump(ETCD_KEY_PREFIX);
}

/**
 * @brief 分配 Kubernetes 节点的子网
 *
 * @param node_name Kubernetes 节点名称
 * @param center Center 对象
 * @param subnet 节点子网（返回值）
 * @return true 分配成功
 * @return false 分配失败
 */
auto Ipam::allocateSubnet(std::string_view node_name, const backend::CenterIf *center,
                          std::string &subnet) -> bool {
  OHNO_ASSERT(etcd_client_);
  OHNO_ASSERT(center != nullptr);

  if (getSubnet(node_name, subnet)) {
    return true;
  }

  auto node_info = center->getKubernetesData(node_name);
  subnet = node_info.pod_cidr_;

  std::string key = std::string{ETCD_KEY_SUBNET};
  if (etcd_client_->append(key, subnet)) {
    if (etcd_client_->put(fmt::format("{}/{}", key, node_name), subnet)) {
      OHNO_LOG(trace, "IPAM allocated subnet:{} for {}", subnet, node_name);
      return true;
    }
    etcd_client_->del(key, subnet);
  }

  subnet.clear();
  return false;
}

/**
 * @brief 删除为 Kubernetes 节点分配的子网
 *
 * @param node_name Kubernetes 节点
 * @param subnet 节点对应子网
 * @return true 删除成功
 * @return false 删除失败
 */
auto Ipam::releaseSubnet(std::string_view node_name, std::string_view subnet) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(etcd_client_);

  // 删除 /ohno/subnets/节点名称
  std::string key = fmt::format("{}/{}", ETCD_KEY_SUBNET, node_name);
  if (!etcd_client_->del(key)) {
    OHNO_LOG(warn, "Failed to release {}/{}", ETCD_KEY_SUBNET, node_name);
    return false;
  }

  // 删除 /ohno/subnets 出现的子网
  key = std::string{ETCD_KEY_SUBNET};
  if (!etcd_client_->del(key, subnet)) {
    OHNO_LOG(warn, "Failed to release subnet {} in {}", subnet, ETCD_KEY_SUBNET);
    return false;
  }

  OHNO_LOG(trace, "IPAM release subnet {} for {}", subnet, node_name);
  return true;
}

/**
 * @brief 获取 Kubernetes 节点所属子网
 *
 * @param node_name Kubernetes 节点名称
 * @param subnet 子网（返回值）
 * @return true 获取成功
 * @return false 获取失败
 */
auto Ipam::getSubnet(std::string_view node_name, std::string &subnet) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(etcd_client_);

  subnet.clear();
  bool ret = etcd_client_->get(fmt::format("{}/{}", ETCD_KEY_SUBNET, node_name), subnet);
  if (!ret) {
    OHNO_LOG(warn, "Failed to get {}/{}", ETCD_KEY_SUBNET, node_name);
    return false;
  }

  return !subnet.empty();
}

/**
 * @brief 分配 Kubernetes 节点的待使用的 IP 地址
 *
 * @param node_name 节点名称
 * @param result_ip 待使用的 IP 地址（返回值）
 * @return true 分配成功
 * @return false 分配失败
 */
auto Ipam::allocateIp(std::string_view node_name, std::string &result_ip) -> bool {
  OHNO_ASSERT(etcd_client_);

  std::string subnet{};
  if (!getSubnet(node_name, subnet)) {
    OHNO_LOG(warn, "Failed to allocate ip because get {}/{} failed", ETCD_KEY_SUBNET, node_name);
    return false;
  }

  net::Subnet subnet_obj{};
  subnet_obj.init(subnet);
  auto max_hosts = subnet_obj.getMaxHosts();

  // 查找可用 IP（从 .1 到 .254）
  for (net::Prefix i = 1; i < max_hosts - 1; ++i) {
    std::string candidate_ip = subnet_obj.generateIp(i);

    if (isIpAvailable(node_name, candidate_ip)) {
      std::string key = fmt::format("{}/{}", ETCD_KEY_ADDRESS, node_name);
      if (etcd_client_->append(key, candidate_ip)) {
        OHNO_LOG(trace, "IPAM allocate IP {} for {}", candidate_ip, node_name);
        result_ip = candidate_ip;
        return true;
      }
    }
  }

  result_ip.clear();
  return false;
}

/**
 * @brief 删除 Kubernetes 节点使用的 IP 地址
 *
 * @param node_name 节点名称
 * @param ip_to_del 已使用的 IP 地址
 * @return true 删除成功
 * @return false 删除失败
 */
auto Ipam::releaseIp(std::string_view node_name, std::string_view ip_to_del) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(etcd_client_);

  // 删除 /ohno/address/${节点名字} 出现的 IP 地址
  std::string key = fmt::format("{}/{}", ETCD_KEY_ADDRESS, node_name);
  if (!etcd_client_->del(key, ip_to_del)) {
    OHNO_LOG(warn, "Failed to release IP {} in {}/{}", ip_to_del, ETCD_KEY_ADDRESS, node_name);
    return false;
  }

  OHNO_LOG(trace, "IPAM release IP {} for {}", ip_to_del, node_name);
  return true;
}

/**
 * @brief 获取 Kubernetes 节点所有已使用的 IP 地址
 *
 * @param node_name Kubernetes 节点名称
 * @param all_ip 所有已使用的 IP 地址（返回值）
 * @return true 获取成功
 * @return false 获取失败
 */
auto Ipam::getAllIp(std::string_view node_name, std::vector<std::string> &all_ip) -> bool {
  OHNO_ASSERT(!node_name.empty());

  if (!etcd_client_->list(fmt::format("{}/{}", ETCD_KEY_ADDRESS, node_name), all_ip)) {
    OHNO_LOG(warn, "Failed to get {}/{}", ETCD_KEY_ADDRESS, node_name);
    return false;
  }

  return !all_ip.empty();
}

/**
 * @brief IP 地址是否未分配
 *
 * @param node_name Kubernetes 节点名称
 * @param ip_to_confirm 待确认的 IP 地址
 * @return true 未分配
 * @return false 已分配
 */
auto Ipam::isIpAvailable(std::string_view node_name, std::string_view ip_to_confirm) const -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!ip_to_confirm.empty());
  OHNO_ASSERT(etcd_client_);

  std::vector<std::string> all_addresses{};
  auto ret = etcd_client_->list(fmt::format("{}/{}", ETCD_KEY_ADDRESS, node_name), all_addresses);
  if (!ret) {
    return true; // 获取失败，假设地址可用
  }

  return std::find(all_addresses.begin(), all_addresses.end(), ip_to_confirm) ==
         all_addresses.end();
}

} // namespace ipam
} // namespace ohno
