// clang-format off
#include "node.h"
#include "macro.h"
#include "netns.h"
#include "src/common/assert.h"
#include "src/net/addr.h"
#include "src/net/bridge.h"
#include "src/net/route.h"
#include "src/net/subnet.h"
// clang-format on

namespace ohno {
namespace ipam {

/**
 * @brief 为 Kubernetes 节点设置名称
 *
 * @param node_name 节点名称
 */
auto Node::setName(std::string_view node_name) -> void {
  OHNO_ASSERT(!node_name.empty());
  name_ = node_name;
}

/**
 * @brief 返回 Kubernetes 节点名称
 *
 * @return std::string 节点名称，还没设置则返回空字符串
 */
auto Node::getName() const -> std::string { return name_; }

/**
 * @brief 向 Kubernetes 节点中增加一个 Linux namespace
 *
 * @param netns_name 网络空间名称
 * @param netns 网络空间对象
 */
auto Node::addNetns(std::string_view netns_name, std::shared_ptr<NetnsIf> netns) -> void {
  netns_.emplace(netns_name, netns);
}

/**
 * @brief 从 Kubernetes 节点中删除一个 Linux namespace
 *
 * @param netns_name 网络空间名称
 */
auto Node::delNetns(std::string_view netns_name) -> void {
  OHNO_ASSERT(!netns_name.empty());
  netns_.erase(netns_name.data());
}

/**
 * @brief 获取 Kubernetes 节点中的 Linux namespace
 *
 * @param netns_name 网络空间名称
 * @return std::shared_ptr<NetnsIf> 网络空间对象
 */
auto Node::getNetns(std::string_view netns_name) const -> std::shared_ptr<NetnsIf> {
  OHNO_ASSERT(!netns_name.empty());
  auto iter = netns_.find(netns_name.data());
  if (iter == netns_.end()) {
    OHNO_LOG(error, "cannot find netns {}", netns_name);
    return nullptr;
  }
  return iter->second;
}

/**
 * @brief 获取 Kubernetes 节点中 Linux namespace 数量
 *
 * @note 总是大于等于 1，因为宿主机的 root namespace 也算在内，
 * 而 root namespace 自 Node 对象创建便存在
 *
 * @return size_t 网络空间数量
 */
auto Node::getNetnsSize() const noexcept -> size_t { return netns_.size(); }

/**
 * @brief 为 Kubernetes 节点分配子网，每个节点只有一个子网
 *
 * @param subnet 子网 CIDR
 */
auto Node::setSubnet(std::string_view subnet) -> void {
  subnet_ = std::make_unique<net::Subnet>();
  subnet_->init(subnet);
}

/**
 * @brief 获取 Kubernetes 节点分配的子网
 *
 * @return std::string 子网 CIDR，还没分配则返回空字符串
 */
auto Node::getSubnet() const -> std::string {
  return subnet_ ? subnet_->getSubnet() : std::string{};
}

/**
 * @brief 为 Kubernetes 节点设置 underlay 地址
 *
 * @param underlay_addr 节点以太网口地址 CIDR
 */
auto Node::setUnderlayAddr(std::string_view underlay_addr) -> void {
  underlay_addr_ = std::make_unique<net::Addr>(underlay_addr);
  OHNO_ASSERT(underlay_addr_->ipVersion() != net::IpVersion::RESERVED); // 校验初始化成功
}

/**
 * @brief 获取 Kubernetes 节点 underlay 地址
 *
 * @return std::string 节点以太网口地址 CIDR，还没设置则返回空字符串
 */
auto Node::getUnderlayAddr() const -> std::string {
  return underlay_addr_ ? underlay_addr_->getCidr() : std::string{};
}

/**
 * @brief 为 Kubernetes 节点设置 underlay 设备
 *
 * @param underlay_dev 节点以太网网卡
 */
auto Node::setUnderlayDev(std::string_view underlay_dev) -> void {
  OHNO_ASSERT(!underlay_dev.empty());
  underlay_dev_ = underlay_dev.data();
}

/**
 * @brief 获取 Kubernetes 节点 underlay 设备
 *
 * @return std::string 节点以太网网卡设备名，还没设置则返回空字符串
 */
auto Node::getUnderlayDev() const -> std::string { return underlay_dev_; }

/**
 * @brief 为 Kubernetes 节点添加静态路由，属于回调函数，触发时机为每个节点添加第一个 Pod 的时候
 *
 * @param nodes 集群内所有节点
 */
auto Node::onStaticRouteAdd(const std::vector<std::shared_ptr<NodeIf>> &nodes) -> void {
  auto via_dev = getUnderlayDev(); // 当前节点所有静态路由必定是从自己的 underlay 设备出去的
  auto host = getNetns(HOST);      // 静态路由总是配置到 root namespace
  auto host_if = host->getNic(via_dev);

  for (const auto &node : nodes) {
    if (node->getName() == getName()) {
      continue; // 跳过自己
    }

    auto node_subnet = node->getSubnet();              // 其他节点的子网
    auto node_underlay_addr = node->getUnderlayAddr(); // 其他节点的 underlay 地址
    if (!host_if->addRoute(
            std::make_shared<net::Route>(node_subnet, node_underlay_addr, via_dev))) {
      OHNO_LOG(critical, "Failed to add static route(dest:{}, via:{}, dev{}) to Kubernetes node {}",
               node_subnet, node_underlay_addr, via_dev, getName());
    }
  }
}

/**
 * @brief 为 Kubernetes 节点删除静态路由，属于回调函数，触发时机为每个节点删除最后一个 Pod 的时候
 *
 * @param nodes 集群内所有节点
 * @param network 已删除的节点所属子网
 * @param via 通过形参二子网的 via 地址
 */
auto Node::onStaticRouteDel(const std::vector<std::shared_ptr<NodeIf>> &nodes,
                            std::string_view network, std::string_view via) -> void {
  auto via_dev = getUnderlayDev(); // 当前节点所有静态路由必定是从自己的 underlay 设备出去的
  auto host = getNetns(HOST);      // 静态路由总是配置到 root namespace
  auto host_if = host->getNic(via_dev);

  for (const auto &node : nodes) {
    if (node->getName() == getName()) {
      continue; // 跳过自己
    }

    if (!host_if->delRoute(network, via, via_dev)) {
      OHNO_LOG(critical,
               "Failed to delete static route(dest:{}, via:{}, dev{}) from Kubernetes node {}",
               network, via, via_dev, getName());
    }
  }
}

} // namespace ipam
} // namespace ohno
