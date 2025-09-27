// clang-format off
#include "nic.h"
#include "spdlog/fmt/fmt.h"
#include "src/common/assert.h"
// clang-format on

namespace ohno {
namespace net {

/**
 * @brief 将网卡设置到系统网络配置中（由派生类实现）
 *
 * @param netlink Netlink 对象
 * @return true 设置成功
 * @return false 设置失败
 */
auto Nic::setup(std::weak_ptr<NetlinkIf> netlink) -> bool {
  netlink_ = std::move(netlink);
  return static_cast<bool>(netlink_.lock());
}

/**
 * @brief 删除 IP 地址，删除路由，并从系统中删除网卡
 *
 */
auto Nic::cleanup() -> void {
  if (auto ntl = netlink_.lock()) {
    for (const auto &route : routes_) {
      if (ntl->routeIsExist(route->getDest(), route->getVia(), route->getDev(), getNetns())) {
        ntl->routeSetEntry(route->getDest(), route->getVia(), false, route->getDev(), getNetns());
      }
    }
    if (getType() == Type::USER) {
      for (const auto &addr : addrs_) {
        ntl->addressSetEntry(getName(), addr->getAddrCidr(), false, getNetns());
      }
      ntl->linkDestory(getName(), getNetns());
    }
  }
}

/**
 * @brief 判断网络接口是否存在
 *
 * @return true 存在
 * @return false 不存在
 */
auto Nic::isExist() const -> bool {
  auto name = getName();
  OHNO_ASSERT(!name.empty());

  if (auto ntl = netlink_.lock()) {
    if (ntl->linkExist(name, getNetns())) {
      return true;
    }
  }

  return false;
}

/**
 * @brief 设置网络接口名称
 *
 * @param name 名称
 */
auto Nic::setName(std::string_view name) -> void {
  OHNO_ASSERT(!name.empty());
  name_ = name.data();
}

/**
 * @brief 获取网络接口名称
 *
 * @return std::string 字符串
 */
auto Nic::getName() const -> std::string { return name_; }

/**
 * @brief 网络接口重命名
 *
 * @param name 新名称
 * @return true 重命名成功
 * @return false 重命名失败
 */
auto Nic::rename(std::string_view name) -> bool {
  if (name.empty()) {
    return true;
  }

  if (auto ntl = netlink_.lock()) {
    if (ntl->linkRename(getName(), name, getNetns())) {
      setName(name);
      return true;
    }
  }

  return false;
}

/**
 * @brief 网络接口加入网络空间
 *
 * @param netns 网络空间名称（支持 /var/run/netns/$NETNS 和 $NETNS 两种格式）
 * @return true 加入成功
 * @return false 加入失败
 */
auto Nic::setNetns(std::string_view netns) -> bool {
  OHNO_ASSERT(!netns.empty());

  if (auto ntl = netlink_.lock()) {
    std::string netns_simple = Nic::simpleNetns(netns);
    auto name = getName();
    if (!ntl->linkIsInNetns(name, netns_simple)) {
      if (!ntl->linkToNetns(name, netns_simple)) {
        return false;
      }
    }
    netns_ = std::string{netns_simple};
    return true;
  }

  return false;
}

/**
 * @brief 获取网络接口对应网络空间
 *
 * @return std::string 网络空间名称，空字符串表示该网络接口属于 root 网络空间
 */
auto Nic::getNetns() const -> std::string { return netns_; }

/**
 * @brief 设置网络接口启用或关闭
 *
 * @param status 网络接口状态
 * @return true 设置成功
 * @return false 设置失败
 */
auto Nic::setStatus(LinkStatus status) -> bool {
  if (auto ntl = netlink_.lock()) {
    if (ntl->linkSetStatus(getName(), status, getNetns())) {
      status_ = status;
      return true;
    }
  }

  return false;
}

/**
 * @brief 获取网络接口启用状态
 *
 * @return true 启用
 * @return false 关闭
 */
auto Nic::getStatus() const noexcept -> bool { return status_ == LinkStatus::UP; }

/**
 * @brief 向网络接口增加一个 IP 地址，该地址写入系统配置中
 *
 * @note 不要添加 Kubernetes 节点 underlay 地址，因为网卡对象析构时会将地址从系统中移除，
 * 即 Kubernetes 节点上会删除这个 underlay 地址，这样会导致整个 Kubernetes 集群 underlay 通信故障
 *
 * @param addr CIDR 格式 IP 地址
 * @return true 添加成功
 * @return false 添加失败
 */
auto Nic::addAddr(std::unique_ptr<AddrIf> addr) -> bool {
  OHNO_ASSERT(addr);
  const auto name = getName();
  const auto addr_cidr = addr->getAddrCidr();
  const auto netns = getNetns();

  if (auto ntl = netlink_.lock()) {
    if (!ntl->addressIsExist(name, addr_cidr, netns)) {
      if (!ntl->addressSetEntry(name, addr_cidr, true, netns)) {
        return false;
      }
    }
    addrs_.emplace_back(std::move(addr));
    return true;
  }

  return false;
}

/**
 * @brief 从网络接口中删除一个 IP 地址，从系统配置中删除该地址
 *
 * @param cidr CIDR 格式 IP 地址
 * @return true 删除成功
 * @return false 删除失败
 */
auto Nic::delAddr(std::string_view cidr) -> bool {
  OHNO_ASSERT(!cidr.empty());
  const auto name = getName();
  const auto netns = getNetns();

  if (auto ntl = netlink_.lock()) {
    if (ntl->addressIsExist(name, cidr, netns)) {
      if (!ntl->addressSetEntry(name, cidr, false, netns)) {
        return false;
      }
      addrs_.erase(
          std::remove_if(addrs_.begin(), addrs_.end(),
                         [cidr](const auto &addr) { return addr->getAddrCidr() == cidr.data(); }),
          addrs_.end());
    }
    return true;
  }

  return false;
}

/**
 * @brief 获取 IP 地址对象
 *
 * @param cidr CIDR 格式 IP 地址（为空时返回数组第一个地址，除非数组为空）
 * @return const AddrIf * IP 地址对象
 */
auto Nic::getAddr(std::string_view cidr) const -> const AddrIf * {
  if (cidr.empty()) {
    if (addrs_.empty()) {
      OHNO_LOG(warn, "Interface {} has no address", getName());
      return nullptr;
    }
    return addrs_.front().get();
  }

  auto iter = std::find_if(addrs_.begin(), addrs_.end(),
                           [cidr](const auto &addr) { return addr->getAddrCidr() == cidr.data(); });

  if (iter != addrs_.end()) {
    return iter->get();
  }
  OHNO_LOG(warn, "Interface {} address {} not found", getName(), cidr);
  return nullptr;
}

/**
 * @brief 向网络接口中添加一条路由，该路由写入系统配置中
 *
 * @param route 路由
 * @return true 添加成功
 * @return false 添加失败
 */
auto Nic::addRoute(std::unique_ptr<RouteIf> route) -> bool {
  OHNO_ASSERT(route);
  const auto dest = route->getDest();
  const auto via = route->getVia();
  const auto dev = route->getDev();

  if (auto ntl = netlink_.lock()) {
    auto netns = getNetns();
    if (!ntl->routeIsExist(dest, via, dev, netns)) {
      if (!ntl->routeSetEntry(dest, via, true, dev, netns)) {
        return false;
      }
    }
    routes_.emplace_back(std::move(route));
    return true;
  }

  return false;
}

/**
 * @brief 从网络接口中删除一条路由，从系统配置中删除该路由
 *
 * @param dst 路由的目的网段
 * @param via 路由的下一条地址
 * @param dev 路由的设备出口
 * @return true 删除成功
 * @return false 删除失败
 */
auto Nic::delRoute(std::string_view dst, std::string_view via, std::string_view dev) -> bool {
  OHNO_ASSERT(!via.empty());

  if (auto ntl = netlink_.lock()) {
    if (ntl->routeIsExist(dst, via, dev, getNetns())) {
      if (!ntl->routeSetEntry(dst, via, false, dev, getNetns())) {
        return false;
      }
      routes_.erase(std::remove_if(routes_.begin(), routes_.end(),
                                   [dst, via, dev](const auto &route) {
                                     return route->getDest() == std::string{dst} &&
                                            route->getVia() == std::string{via} &&
                                            route->getDev() == std::string{dev};
                                   }),
                    routes_.end());
    }
    return true;
  }

  return false;
}

/**
 * @brief 从网络接口中获取一个路由对象
 *
 * @param dst 路由的目的网段
 * @param via 路由的下一条地址
 * @param dev 路由的设备出口
 * @return const RouteIf * 路由对象
 */
auto Nic::getRoute(std::string_view dst, std::string_view via, std::string_view dev) const
    -> const RouteIf * {
  OHNO_ASSERT(!dst.empty());
  OHNO_ASSERT(!via.empty());
  OHNO_ASSERT(!dev.empty());

  auto iter = std::find_if(routes_.begin(), routes_.end(), [dst, via, dev](const auto &route) {
    return route->getDest() == dst.data() && route->getVia() == via.data() &&
           route->getDev() == dev.data();
  });
  if (iter != routes_.end()) {
    return iter->get();
  }
  OHNO_LOG(warn, "no route found for dst={}, via={}, dev={} in NIC={}", dst, via, dev, getName());
  return nullptr;
}

auto Nic::getType() const noexcept -> Type { return type_; }

/**
 * @brief 精简 namespace 命名，将 /var/run/netns/$NETNS 转换为 $NETNS
 *
 * @param netns 网络空间名称
 * @return std::string 简化版
 */
auto Nic::simpleNetns(std::string_view netns) -> std::string {
  std::string ret{};
  if (netns.find(PATH_NAMESPACE) != std::string::npos) {
    ret = netns.substr(strlen(PATH_NAMESPACE.data()) + 1);
  } else {
    ret = netns.data();
  }
  return ret;
}

} // namespace net
} // namespace ohno
