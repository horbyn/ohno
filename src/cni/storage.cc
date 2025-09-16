// clang-format off
#include "storage.h"
#include "spdlog/fmt/fmt.h"
#include "src/common/assert.h"
#include "src/net/addr.h"
#include "src/net/nic.h"
#include "src/net/route.h"
#include "src/helper/string.h"
// clang-format on

namespace ohno {
namespace cni {

/**
 * @brief 持久化初始化
 *
 * @param etcd_client Etcd 客户端
 * @return true 初始化成功
 * @return false 初始化失败
 */
auto Storage::init(std::unique_ptr<etcd::EtcdClientIf> etcd_client) -> bool {
  etcd_client_ = std::move(etcd_client);
  if (etcd_client_ == nullptr) {
    OHNO_LOG(warn, "ETCD client initialization failed");
    return false;
  }
  return true;
}

/**
 * @brief 将持久化信息全部输出出来
 *
 * @return std::string 持久化结果，无结果为空
 */
auto Storage::dump() const -> std::string {
  OHNO_ASSERT(etcd_client_);
  return etcd_client_->dump(ETCD_KEY_PREFIX);
}

/**
 * @brief 将 Pod 对应的 namespace 信息持久化
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param netns_name 网络空间名称
 * @return true 添加成功
 * @return false 添加失败
 */
auto Storage::addNetns(std::string_view node_name, std::string_view pod_name,
                       std::string_view netns_name) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(!netns_name.empty());
  OHNO_ASSERT(etcd_client_);

  auto key = Storage::getNetnsKey(node_name, pod_name);
  if (etcd_client_->put(key, netns_name)) { // 一个 Pod 容器只对应一个 namespace，用 put
    OHNO_LOG(trace, "Storage add namespace:{} for Pod:{} of Kubernetes node:{}", netns_name,
             pod_name, node_name);
    return true;
  }
  return false;
}

/**
 * @brief 删除持久化 namespace 信息
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @return true 删除成功
 * @return false 删除失败
 */
auto Storage::delNetns(std::string_view node_name, std::string_view pod_name) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(etcd_client_);
  if (etcd_client_->del(Storage::getNetnsKey(node_name, pod_name))) {
    OHNO_LOG(trace, "Storage del namespace for Pod:{} of Kubernetes node:{}", pod_name, node_name);
    return true;
  }
  return false;
}

/**
 * @brief 获取持久化 namespace 信息
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @return std::string 网络空间名称
 */
auto Storage::getNetns(std::string_view node_name, std::string_view pod_name) -> std::string {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(etcd_client_);

  std::string ret{};
  auto key = Storage::getNetnsKey(node_name, pod_name);
  etcd_client_->get(key, ret); // 一个 Pod 容器只对应一个 namespace，用 get
  return ret;
}

/**
 * @brief 将 namespace 对应的 Pod 信息持久化
 *
 * @param node_name Kubernetes 节点名称
 * @param netns_name 网络空间名称
 * @param pod_name Pod 名称
 * @return true 添加成功
 * @return false 添加失败
 */
auto Storage::addPod(std::string_view node_name, std::string_view netns_name,
                     std::string_view pod_name) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!netns_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(etcd_client_);

  auto key = Storage::getSinglePodKey(node_name, netns_name);
  if (etcd_client_->put(key, pod_name)) { // 一个 namespace 只对应一个 Pod 容器，用 put
    key = Storage::getAllPodsKey(node_name);
    if (etcd_client_->append(key, pod_name)) {
      OHNO_LOG(trace, "Storage add Pod:{} for namespace:{} of Kubernetes node:{}", pod_name,
               netns_name, node_name);
      return true;
    }
  }
  return false;
}

/**
 * @brief 删除 namespace 对应的 Pod 信息持久化
 *
 * @param node_name Kubernetes 节点名称
 * @param netns_name 网络空间名称
 * @return true 删除成功
 * @return false 删除失败
 */
auto Storage::delPod(std::string_view node_name, std::string_view netns_name) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!netns_name.empty());
  OHNO_ASSERT(etcd_client_);

  auto key = Storage::getSinglePodKey(node_name, netns_name);
  std::string pod_name{};
  if (etcd_client_->get(key, pod_name)) {
    if (etcd_client_->del(key)) {
      key = Storage::getAllPodsKey(node_name);
      if (etcd_client_->del(key, pod_name)) {
        OHNO_LOG(trace, "Storage del namespace:{} of Kubernetes node:{}", netns_name, node_name);
        return true;
      }
    }
  }
  return false;
}

/**
 * @brief 获取 namespace 对应的 Pod 信息持久化
 *
 * @param node_name Kubernetes 节点名称
 * @param netns_name 网络空间名称
 * @return std::string Pod 名称
 */
auto Storage::getPod(std::string_view node_name, std::string_view netns_name) -> std::string {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!netns_name.empty());
  OHNO_ASSERT(etcd_client_);

  std::string ret{};
  auto key = Storage::getSinglePodKey(node_name, netns_name);
  etcd_client_->get(key, ret); // 一个 namespace 只对应一个 Pod 容器，用 get
  return ret;
}

/**
 * @brief 获取所有 Pod 持久化
 *
 * @param node_name Kubernetes 节点名称
 * @return std::string 对应节点上所有 Pod 名称
 */
auto Storage::getAllPods(std::string_view node_name) -> std::vector<std::string> {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(etcd_client_);

  std::vector<std::string> vec{};
  auto key = Storage::getAllPodsKey(node_name);
  etcd_client_->list(key, vec);
  return vec;
}

/**
 * @brief 网卡信息持久化（信息组织方式 key-value，其中 value 格式为逗号分割的字符串）
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name 网卡设备名
 * @return true 成功
 * @return false 失败
 */
auto Storage::addNic(std::string_view node_name, std::string_view pod_name,
                     std::string_view nic_name) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(etcd_client_);

  auto key = Storage::getNicKey(node_name, pod_name);
  if (etcd_client_->append(key, nic_name)) {
    OHNO_LOG(trace, "Storage add NIC:{} for Pod:{} of Kubernetes node:{}", nic_name, pod_name,
             node_name);
    return true;
  }
  return false;
}

/**
 * @brief 删除持久化网卡信息
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name 网卡名称
 * @return true 删除成功
 * @return false 删除失败
 */
auto Storage::delNic(std::string_view node_name, std::string_view pod_name,
                     std::string_view nic_name) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(etcd_client_);
  if (etcd_client_->del(Storage::getNicKey(node_name, pod_name), nic_name)) {
    OHNO_LOG(trace, "Storage del NIC:{} for Pod:{} of Kubernetes node:{}", nic_name, pod_name,
             node_name);
    return true;
  }
  return false;
}

/**
 * @brief 获取持久化网卡信息
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @return std::vector<std::string> 网卡数组
 */
auto Storage::getAllNic(std::string_view node_name, std::string_view pod_name) const
    -> std::vector<std::string> {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(etcd_client_);

  std::vector<std::string> vec{};
  auto key = Storage::getNicKey(node_name, pod_name);
  etcd_client_->list(key, vec);
  return vec;
}

/**
 * @brief 地址信息持久化
 *
 * @note 信息组织方式 key-value，其中 value 格式为逗号分割的字符串如 addr-dev
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name 网卡设备名
 * @param addr IP 地址对象
 * @return true 成功
 * @return false 失败
 */
auto Storage::addAddr(std::string_view node_name, std::string_view pod_name,
                      std::string_view nic_name, std::unique_ptr<net::AddrIf> addr) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(addr);
  OHNO_ASSERT(etcd_client_);

  auto key = Storage::getAddrKey(node_name, pod_name, nic_name);
  auto addr_str = addr->getCidr();
  if (etcd_client_->append(key, addr_str)) {
    OHNO_LOG(trace, "Storage add Addr:{} for NIC:{} for Pod:{} of Kubernetes node:{}", addr_str,
             nic_name, pod_name, node_name);
    return true;
  }
  return false;
}

/**
 * @brief 删除持久化地址信息
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name 网卡名称
 * @return true 删除成功
 * @return false 删除失败
 */
auto Storage::delAddr(std::string_view node_name, std::string_view pod_name,
                      std::string_view nic_name) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(etcd_client_);
  if (etcd_client_->del(Storage::getAddrKey(node_name, pod_name, nic_name))) {
    OHNO_LOG(trace, "Storage del Addr for NIC:{} for Pod:{} of Kubernetes node:{}", nic_name,
             pod_name, node_name);
    return true;
  }
  return false;
}

/**
 * @brief 获取持久化 IP 地址信息
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name 网卡设备名
 * @return std::vector<std::string> 地址对象数组
 */
auto Storage::getAllAddrs(std::string_view node_name, std::string_view pod_name,
                          std::string_view nic_name) const -> std::vector<std::string> {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(etcd_client_);

  auto key = Storage::getAddrKey(node_name, pod_name, nic_name);
  std::vector<std::string> vec{};
  etcd_client_->list(key, vec);
  return vec;
}

/**
 * @brief 路由信息持久化
 *
 * @note 信息组织方式 key-value，其中 value 格式为逗号分割的字符串如 dest-via-dev
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name 网卡设备名
 * @param route 路由对象
 * @return true 成功
 * @return false 失败
 */
auto Storage::addRoute(std::string_view node_name, std::string_view pod_name,
                       std::string_view nic_name, std::unique_ptr<net::RouteIf> route) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(route);
  OHNO_ASSERT(etcd_client_);

  auto key = Storage::getRouteKey(node_name, pod_name, nic_name);
  auto dest = route->getDest();
  auto via = route->getVia();
  auto dev = route->getDev();
  auto value = Storage::getRouteValue(dest, via, dev);
  if (etcd_client_->append(key, value)) {
    OHNO_LOG(
        trace,
        "Storage add Route:{dest:{}, via:{}, dev:{}} for NIC:{} for Pod:{} of Kubernetes node:{}",
        dest, via, dev, nic_name, pod_name, node_name);
    return true;
  }
  return false;
}

/**
 * @brief 删除持久化路由信息
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name 网卡名称
 * @return true 删除成功
 * @return false 删除失败
 */
auto Storage::delRoute(std::string_view node_name, std::string_view pod_name,
                       std::string_view nic_name) -> bool {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(etcd_client_);
  if (etcd_client_->del(Storage::getRouteKey(node_name, pod_name, nic_name))) {
    OHNO_LOG(trace, "Storage del Route for NIC:{} for Pod:{} of Kubernetes node:{}", nic_name,
             pod_name, node_name);
    return true;
  }
  return false;
}

/**
 * @brief 获取持久化路由信息
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name 网卡设备名
 * @return std::vector<std::string> 路由对象数组
 */
auto Storage::getAllRoutes(std::string_view node_name, std::string_view pod_name,
                           std::string_view nic_name) const
    -> std::vector<std::unique_ptr<net::RouteIf>> {
  OHNO_ASSERT(!node_name.empty());
  OHNO_ASSERT(!pod_name.empty());
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(etcd_client_);

  auto key = Storage::getRouteKey(node_name, pod_name, nic_name);
  std::vector<std::string> vec{};
  etcd_client_->list(key, vec);

  std::vector<std::unique_ptr<net::RouteIf>> ret{};
  for (const auto &item : vec) {
    // TODO: 单元测试验证下 空-空-空 会怎样
    auto route = helper::split(item, SEPARATOR); // 0:dest, 1:via, 2:dev
    OHNO_ASSERT(route.size() == 3);
    ret.emplace_back(std::make_unique<net::Route>(route[0], route[1], route[2]));
  }
  return ret;
}

/**
 * @brief 获取持久化网络空间 key
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @return std::string ETCD key
 */
auto Storage::getNetnsKey(std::string_view node_name, std::string_view pod_name) -> std::string {
  return fmt::format("{}/{}/pod/{}/netns", ETCD_KEY_PREFIX, node_name, pod_name);
}

/**
 * @brief 获取持久化单个 Pod 名称 key
 *
 * @param node_name Kubernetes 节点名称
 * @param netns_name 网络空间名称
 * @return std::string ETCD key
 */
auto Storage::getSinglePodKey(std::string_view node_name, std::string_view netns_name)
    -> std::string {
  return fmt::format("{}/{}/netns/{}/pod", ETCD_KEY_PREFIX, node_name,
                     net::Nic::simpleNetns(netns_name));
}

/**
 * @brief 获取持久化全部 Pod 名称 key
 *
 * @param node_name Kubernetes 节点名称
 * @return std::string ETCD key
 */
auto Storage::getAllPodsKey(std::string_view node_name) -> std::string {
  return fmt::format("{}/{}/pod", ETCD_KEY_PREFIX, node_name);
}

/**
 * @brief 获取持久化 NIC key
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @return std::string ETCD key
 */
auto Storage::getNicKey(std::string_view node_name, std::string_view pod_name) -> std::string {
  return fmt::format("{}/{}/pod/{}/nic", ETCD_KEY_PREFIX, node_name, pod_name);
}

/**
 * @brief 获取持久化地址 key
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name NIC 设备名
 * @return std::string ETCD key
 */
auto Storage::getAddrKey(std::string_view node_name, std::string_view pod_name,
                         std::string_view nic_name) -> std::string {
  return fmt::format("{}/{}/pod/{}/nic/{}/addr", ETCD_KEY_PREFIX, node_name, pod_name, nic_name);
}

/**
 * @brief 获取持久化路由 key
 *
 * @param node_name Kubernetes 节点名称
 * @param pod_name Pod 名称
 * @param nic_name NIC 设备名
 * @return std::string ETCD key
 */
auto Storage::getRouteKey(std::string_view node_name, std::string_view pod_name,
                          std::string_view nic_name) -> std::string {
  return fmt::format("{}/{}/pod/{}/nic/{}/route", ETCD_KEY_PREFIX, node_name, pod_name, nic_name);
}

/**
 * @brief 获取路由条目
 *
 * @param dest 目的网段
 * @param via 下一跳地址
 * @param dev 经过设备
 * @return std::string 路由条目
 */
auto Storage::getRouteValue(std::string_view dest, std::string_view via, std::string_view dev)
    -> std::string {
  OHNO_ASSERT(!via.empty());
  OHNO_ASSERT(!dev.empty());

  // 路由格式为 dest-via-dev，中间用短横杠连接
  return fmt::format("{}{}{}{}{}", dest, SEPARATOR, via, SEPARATOR, dev);
}

} // namespace cni
} // namespace ohno
