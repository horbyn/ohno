// clang-format off
#include "node.h"
#include "macro.h"
#include "netns.h"
#include "src/common/assert.h"
#include "src/common/except.h"
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
  OHNO_ASSERT(!netns_name.empty());
  OHNO_ASSERT(netns);
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
auto Node::getNetnsSize() const noexcept -> size_t {
  OHNO_ASSERT(!netns_.empty());
  return netns_.size();
}

/**
 * @brief 为 Kubernetes 节点分配子网，每个节点只有一个子网
 *
 * @param subnet 子网 CIDR
 */
auto Node::setSubnet(std::string_view subnet) -> void {
  OHNO_ASSERT(!subnet.empty());
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
  OHNO_ASSERT(!underlay_addr.empty());
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

} // namespace ipam
} // namespace ohno
