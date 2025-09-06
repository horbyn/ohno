// clang-format off
#include "nic.h"
#include <fcntl.h>
#include <net/if.h>
#include "spdlog/fmt/fmt.h"
#include "src/common/assert.h"
// clang-format on

namespace ohno {
namespace net {

Nic::Nic(const std::shared_ptr<NetlinkIf> &netlink) : netlink_{netlink} {}

/**
 * @brief 设置网络接口名称
 *
 * @param name 名称
 */
auto Nic::setName(std::string_view name) -> void { name_ = name.data(); }

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
  } else {
    OHNO_LOG(error, "Failed to rename interface:{} to {}", getName(), name);
  }
  return false;
}

/**
 * @brief 网络接口加入网络空间
 *
 * @param netns 网络空间名称（支持 /var/run/netns/$NETNS 和 $NETNS 两种格式）
 */
auto Nic::addNetns(std::string_view netns) -> void {
  OHNO_ASSERT(!netns.empty());

  if (netns.find(PATH_NAMESPACE) != std::string::npos) {
    netns_ = netns.substr(strlen(PATH_NAMESPACE.data()) + 1);
  } else {
    netns_ = netns.data();
  }
}

/**
 * @brief 删除网络空间
 *
 */
auto Nic::delNetns() -> void { netns_.clear(); }

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
  } else {
    OHNO_LOG(error, "Failed to set {} of NIC {}", (status == LinkStatus::UP ? "up" : "down"),
             getName());
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
 * @brief 获取网络接口的索引
 *
 * @return uint32_t 索引号，获取失败返回 -1
 */
auto Nic::getIndex() const -> uint32_t {

  if (!netns_.empty()) {
    // TODO: open() 返回的 fd 应该用 RAII 管理
    int file_desc = open(fmt::format("{}/{}", PATH_NAMESPACE, netns_).c_str(), O_RDONLY);
    if (file_desc == -1) {
      OHNO_LOG(error, "Failed to open namespace {}", netns_);
      return -1;
    }

    if (setns(file_desc, CLONE_NEWNET) == -1) {
      OHNO_LOG(error, "Failed to switch to namespace {}", netns_);
      close(file_desc);
      return -1;
    }
    close(file_desc);
  }

  auto ifindex = if_nametoindex(getName().data());
  if (ifindex == 0) {
    OHNO_LOG(error, "Interface {} index not found", getName());
    return -1;
  }

  return ifindex;
}

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
auto Nic::addAddr(std::shared_ptr<AddrIf> addr) -> bool {
  OHNO_ASSERT(addr);

  if (auto ntl = netlink_.lock()) {
    if (ntl->addressSetEntry(getName(), addr->getCidr(), true, getNetns())) {
      addrs_.emplace_back(addr);
      return true;
    }
  } else {
    OHNO_LOG(error, "Failed to add address {} to interface {}", addr->getCidr(), getName());
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

  if (auto ntl = netlink_.lock()) {
    if (ntl->addressSetEntry(getName(), cidr, false, getNetns())) {
      addrs_.erase(std::remove_if(addrs_.begin(), addrs_.end(),
                                  [cidr](const std::shared_ptr<AddrIf> &addr) {
                                    return addr->getCidr() == cidr.data();
                                  }),
                   addrs_.end());
      return true;
    }
  } else {
    OHNO_LOG(error, "Failed to del address {} to interface {}", cidr, getName());
  }
  return false;
}

/**
 * @brief 获取 IP 地址对象
 *
 * @param cidr CIDR 格式 IP 地址（为空时返回数组第一个地址，除非数组为空）
 * @return std::shared_ptr<AddrIf> IP 地址对象
 */
auto Nic::getAddr(std::string_view cidr) const -> std::shared_ptr<AddrIf> {
  if (cidr.empty()) {
    if (addrs_.empty()) {
      OHNO_LOG(warn, "Interface {} has no address", getName());
      return nullptr;
    }
    return addrs_.front();
  }

  auto iter =
      std::find_if(addrs_.begin(), addrs_.end(), [cidr](const std::shared_ptr<AddrIf> &addr) {
        return addr->getCidr() == cidr.data();
      });

  if (iter != addrs_.end()) {
    return *iter;
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
auto Nic::addRoute(std::shared_ptr<RouteIf> route) -> bool {
  OHNO_ASSERT(route);
  if (auto ntl = netlink_.lock()) {
    if (!ntl->routeIsExist(route->getDest(), route->getVia(), route->getDev(), getNetns())) {
      if (ntl->routeSetEntry(route->getDest(), route->getVia(), true, route->getDev(),
                             getNetns())) {
        routes_.emplace_back(route);
        return true;
      }
    }
  } else {
    OHNO_LOG(error, "Failed to add route {dest:{}, via: {}, dev: {}} to interface {}",
             route->getDest(), route->getVia(), route->getDev(), getName());
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
  OHNO_ASSERT(!dst.empty());
  OHNO_ASSERT(!via.empty());
  OHNO_ASSERT(!dev.empty());

  if (auto ntl = netlink_.lock()) {
    if (ntl->routeSetEntry(dst, via, false, dev, getNetns())) {
      routes_.erase(std::remove_if(routes_.begin(), routes_.end(),
                                   [dst, via, dev](const std::shared_ptr<RouteIf> &route) {
                                     return route->getDest() == dst.data() &&
                                            route->getVia() == via.data() &&
                                            route->getDev() == dev.data();
                                   }),
                    routes_.end());
      return true;
    }
  } else {
    OHNO_LOG(error, "Failed to del route {dest:{}, via:{}, dev:{}} to interface {}", dst, via, dev,
             getName());
  }
  return false;
}

/**
 * @brief 从网络接口中获取一个路由对象
 *
 * @param dst 路由的目的网段
 * @param via 路由的下一条地址
 * @param dev 路由的设备出口
 * @return std::shared_ptr<RouteIf> 路由对象
 */
auto Nic::getRoute(std::string_view dst, std::string_view via, std::string_view dev) const
    -> std::shared_ptr<RouteIf> {
  OHNO_ASSERT(!dst.empty());
  OHNO_ASSERT(!via.empty());
  OHNO_ASSERT(!dev.empty());

  auto iter = std::find_if(routes_.begin(), routes_.end(),
                           [dst, via, dev](const std::shared_ptr<RouteIf> &route) {
                             return route->getDest() == dst.data() &&
                                    route->getVia() == via.data() && route->getDev() == dev.data();
                           });
  if (iter != routes_.end()) {
    return *iter;
  }
  OHNO_LOG(warn, "no route found for dst={}, via={}, dev={} in NIC={}", dst, via, dev, getName());
  return nullptr;
}

/**
 * @brief 删除 IP 地址，删除路由，并从系统中删除网卡
 *
 */
auto Nic::cleanup() noexcept -> void {
  try {
    if (auto ntl = netlink_.lock()) {
      for (const auto &addr : addrs_) {
        ntl->addressSetEntry(getName(), addr->getCidr(), false, getNetns());
      }
      for (const auto &route : routes_) {
        if (ntl->routeIsExist(route->getDest(), route->getVia(), route->getDev(), getNetns())) {
          ntl->routeSetEntry(route->getDest(), route->getVia(), false, route->getDev(), getNetns());
        }
      }
      if (!ntl->linkDestory(getName(), getNetns())) {
        OHNO_LOG(warn, "Failed to destroy NIC: {}", getName());
      }
    }
  } catch (const std::exception &err) {
    OHNO_LOG(warn, "Failed to release NIC: {}", err.what());
  }
}

} // namespace net
} // namespace ohno
