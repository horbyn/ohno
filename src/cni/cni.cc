// clang-format off
#include "cni.h"
#include <iostream>
#include "cni_result.h"
#include "spdlog/fmt/fmt.h"
#include "src/common/assert.h"
#include "src/common/except.h"
#include "src/ipam/macro.h"
#include "src/ipam/netns.h"
#include "src/ipam/node.h"
#include "src/net/addr.h"
#include "src/net/bridge.h"
#include "src/net/route.h"
#include "src/net/subnet.h"
#include "src/net/veth.h"
#include "src/net/netlink/netlink_ip_cmd.h"
#include "src/util/env_std.h"
#include "src/util/shell_sync.h"
// clang-format on

namespace ohno {
namespace cni {

Cni::Cni(std::unique_ptr<net::NetlinkIf> netlink) : netlink_{std::move(netlink)} {}

/**
 * @brief CNI 配置初始化
 *
 * @param conf CNI 配置
 */
auto Cni::parseConfig(const CniConfig &conf) -> void {
  // CNI 配置文件
  conf_ = conf;
}

/**
 * @brief CNI 配置初始化
 *
 * @param json CNI 配置 JSON
 * @return true 初始化成功
 * @return false 初始化失败
 */
auto Cni::parseConfig(const nlohmann::json &json) -> bool {
  OHNO_ASSERT(!json.empty());

  try {
    CniConfig cni_conf = json;
    parseConfig(cni_conf);
  } catch (const std::exception &json_err) {
    OHNO_LOG(error, "Failed to parse CNI config from json format: \"{}\"", json.dump());
    return false;
  }

  return true;
}

/**
 * @brief 集群初始化
 *
 * @param cluster 集群对象
 * @return true 初始化成功
 * @return false 初始化失败
 */
auto Cni::setCluster(std::unique_ptr<ipam::ClusterIf> cluster) -> bool {
  cluster_ = std::move(cluster);
  if (!cluster_) {
    OHNO_LOG(error, "Failed to set Kubernetes cluster");
    return false;
  }
  return true;
}

/**
 * @brief IPAM 初始化
 *
 * @param ipam IPAM 对象
 * @return true 初始化成功
 * @return false 初始化失败
 */
auto Cni::setIpam(std::unique_ptr<ipam::IpamIf> ipam) -> bool {
  ipam_ = std::move(ipam);
  if (!ipam_) {
    OHNO_LOG(error, "Failed to set ipam");
    return false;
  }
  return true;
}
/**
 * @brief CNI ADD
 *
 * @param container_id 来自环境变量 CNI_CONTAINERID
 * @param netns 来自环境变量 CNI_NETNS
 * @param nic_name 来自环境变量 CNI_IFNAME
 * @return std::string 一个 json 格式字符串
 */
auto Cni::add(std::string_view container_id, std::string_view netns, std::string_view nic_name)
    -> std::string {
  OHNO_ASSERT(!container_id.empty());
  OHNO_ASSERT(!netns.empty());
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(cluster_);
  OHNO_ASSERT(ipam_);
  OHNO_LOG(debug, "CNI ADD parameters: container_id:\"{}\", netns:\"{}\", nic_name:\"{}\"",
           container_id, netns, nic_name);

  std::string node_name{};
  std::string node_underlay_dev{};
  std::string node_underlay_addr{};
  if (!getCurrentNodeInfo(node_name, node_underlay_dev, node_underlay_addr)) {
    throw OHNO_EXCEPT("Failed to get current node info", false);
  }
  auto netlink_if = std::shared_ptr<net::NetlinkIf>(netlink_.get());
  auto netlink = std::dynamic_pointer_cast<net::NetlinkIpCmd>(netlink_if);
  if (!netlink) {
    throw OHNO_EXCEPT("Failed to create netlink interface", false);
  }

  // 获取 Kubernetes 节点
  auto node = getKubernetesNode(node_name, netlink, node_underlay_dev, node_underlay_addr);
  if (!node) {
    throw OHNO_EXCEPT(fmt::format("Failed to get Kubernetes node:{}", node_name), false);
  }

  // 获取 Kubernetes Pod
  // 因为 container_id 是 pause 容器的标识，所以可以用来唯一标识一个 Pod
  auto pod = getKubernetesPod(node, container_id, true);
  if (!pod) {
    throw OHNO_EXCEPT(
        fmt::format("Failed to get Kubernetes pod:{} of node:{}", container_id, node_name), false);
  }

  // 获取 Pod 网卡（Veth）
  // pod 一端使用 $CNI_IFNAME 名称，宿主机一端使用 veth_$CNI_CONTAINERID 名称
  auto veth_host = fmt::format("veth_{}", container_id);
  auto veth_pod = nic_name;
  bool flag_add = true;
  auto gateway = conf_.plugins_.ipam_.gateway_;
  std::string pod_addr{};

  auto nic = getKubernetesNic(pod, veth_pod); // 每个 Pod 只保留一张网卡，因为 pod 网络是共享的
  if (nic) {
    // TODO:
    // 如果 CNI 配置给出的子网和 gateway 与 Pod 已生效的网络配置不一样时，此时怎么做？
    // 所有 Pod 都要删除？但 CNI ADD 只是增加一个 Pod，则已删除的其他 Pod 由 CRI
    // 负责重新向 CNI 发命令创建吗？
    net::Subnet subnet_cni_conf{};
    subnet_cni_conf.init(conf_.plugins_.ipam_.subnet_);
    net::Subnet subnet_veth{};
    auto addr_obj = nic->getAddr();
    if (!addr_obj) {
      throw OHNO_EXCEPT(fmt::format("Failed to get veth address:{}", veth_pod), false);
    }
    subnet_veth.init(addr_obj->getCidr());
    if (subnet_cni_conf != subnet_veth) {
      throw OHNO_EXCEPT(fmt::format("CNI subnet:{} changes", conf_.plugins_.ipam_.subnet_), false);
    }

    // 之前已经在 Pod 创建过，但现在网卡名称不一样了，所以找不到网卡
    if (!nic->rename(veth_pod)) {
      throw OHNO_EXCEPT(
          fmt::format("Failed to rename veth:{} to veth:{}", nic->getName(), veth_pod), false);
    }
    flag_add = false;
  } else {
    // 这个 Pod 第一次创建网卡
    nic = getKubernetesNic(pod, veth_pod, netlink, node_name, veth_host, netns);
    if (!nic) {
      throw OHNO_EXCEPT(fmt::format("Failed to get Kubernetes veth pair on pod:{} of node:{}",
                                    container_id, node_name),
                        false);
    }
    pod->addNic(nic);
  }

  // 创建宿主机静态路由
  if (flag_add && node->getNetnsSize() == 2) {
    // 算上宿主机的 root namespace，数量为 2 说明节点现在才创建第一个 Pod
    cluster_->NotifyAdd();
  }

  // 输出
  auto addr_obj =
      nic->getAddr(); // 根据 CNI spec：
                      // https://github.com/containernetworking/cni.dev/blob/main/content/docs/spec.md#add-success,
                      // 一个网卡目前只有一个地址
  pod_addr = addr_obj ? std::string{UNKNOWN_ADDR_V4} : addr_obj->getCidr();
  CniResult result{.cniversion_ = conf_.cni_version_,
                   .ips_ = std::vector<CniResultIps>{CniResultIps{
                       .address_ = pod_addr, .gateway_ = gateway, .interface_ = nic->getIndex()}},
                   .interfaces_ = std::vector<CniResultInterfaces>{
                       CniResultInterfaces{.name_ = veth_pod.data(), .sandbox_ = netns.data()}}};
  return nlohmann::json(result).dump();
}

/**
 * @brief CNI DEL
 *
 * @param container_id 来自环境变量 CNI_CONTAINERID
 * @param nic_name 来自环境变量 CNI_IFNAME
 */
auto Cni::del(std::string_view container_id, std::string_view nic_name) noexcept -> void {
  try {
    OHNO_ASSERT(!container_id.empty());
    OHNO_ASSERT(!nic_name.empty());

    OHNO_LOG(debug, "CNI ADD parameters: container_id:\"{}\", nic_name:\"{}\"", container_id,
             nic_name);

    std::string node_name{};
    std::string node_underlay_dev{};
    std::string node_underlay_addr{};
    if (!getCurrentNodeInfo(node_name, node_underlay_dev, node_underlay_addr)) {
      throw OHNO_EXCEPT("Failed to get current node info", false);
    }

    // 删除 Pod 网络接口
    auto node = getKubernetesNode(node_name);
    if (!node) {
      throw OHNO_EXCEPT(fmt::format("Failed to get Kubernetes node:{}", node_name), false);
    }

    auto pod = getKubernetesPod(node, container_id);
    if (!pod) {
      OHNO_LOG(warn, "CNI DEL: Pod had been deleted");
    } else {
      delKubernetesNic(pod, node_name, nic_name);
      delKubernetesPod(node, container_id);
    }

    // 删除节点静态路由
    if (node->getNetnsSize() == 1) {
      // 算上宿主机的 root namespace，数量为 1 说明节点刚才删除了最后一个 pod

      // 删除节点 root namespace bridge
      auto host = getKubernetesPod(node, ipam::HOST);
      if (host) {
        delKubernetesNic(host, node_name, conf_.plugins_.bridge_);
      }

      // 归还节点子网
      auto node_subnet = node->getSubnet();
      if (!node_subnet.empty() && !ipam_->releaseSubnet(node_name, node_subnet)) {
        OHNO_LOG(error, "Failed to release node subnet");
      }
      cluster_->NotifyDel(node_subnet, node_underlay_addr);
    }
  } catch (const std::exception &err) {
    OHNO_LOG(error, "CNI DEL failed: {}", err.what());
  }
}

/**
 * @brief CNI VERSION
 *
 * @return std::string 版本 json 字符串
 */
auto Cni::version() const -> std::string {
  CniVersion ver{};
  ver.cni_version_ = conf_.cni_version_;
  return nlohmann::json(ver).dump();
}

/**
 * @brief 获取当前 Kubernetes 节点信息
 *
 * @param node_name 节点名称
 * @param underlay_dev 节点 Underlay 设备名
 * @param underlay_addr 节点 Underlay 地址
 * @return true 获取成功
 * @return false 获取失败
 */
auto Cni::getCurrentNodeInfo(std::string &node_name, std::string &underlay_dev,
                             std::string &underlay_addr) const -> bool {

  auto shell = std::make_shared<util::ShellSync>();
  if (!shell) {
    OHNO_LOG(critical, "Failed to create shell object");
    return false;
  }

  auto ret = shell->execute("hostname", node_name);
  if (!ret || node_name.empty()) {
    OHNO_LOG(critical, "Failed to get current Kubernetes node name");
    return false;
  }

  ret = shell->execute("ip route show default | awk '/default/ {print $5}'", underlay_dev);
  if (!ret || underlay_dev.empty()) {
    OHNO_LOG(critical, "Failed to get current Kubernetes underlay device");
    return false;
  }

  ret = shell->execute(
      fmt::format("ip addr show {} | grep 'inet ' | awk '{{print $2}}' | cut -d'/' -f1",
                  underlay_dev),
      underlay_addr);
  if (!ret || underlay_addr.empty()) {
    OHNO_LOG(critical, "Failed to get current Kubernetes underlay address");
    return false;
  }

  return true;
}

/**
 * @brief 获取 Kubernetes 节点
 *
 * @param node_name 节点名称
 * @param netlink Netlink 对象（可以为空，为空时不存在也不创建节点）
 * @param node_underlay_dev 节点 underlay 网卡（可以为空，为空时不存在也不创建节点）
 * @param node_underlay_addr 节点 underlay 地址（可以为空，为空时不存在也不创建节点）
 * @return std::shared_ptr<ipam::NodeIf> 节点对象，不存在时返回 nullptr
 */
auto Cni::getKubernetesNode(std::string_view node_name,
                            const std::shared_ptr<net::NetlinkIf> &netlink,
                            std::string_view node_underlay_dev, std::string_view node_underlay_addr)
    -> std::shared_ptr<ipam::NodeIf> {
  auto node = cluster_->getNode(node_name);
  if (!node) {
    if (netlink) {
      // 还没有 Kubernetes 节点的抽象
      node = std::make_shared<ipam::Node>();
      if (!node) {
        throw OHNO_EXCEPT(fmt::format("Failed to create Kubernetes node on node:{}", node_name),
                          false);
      }

      std::string_view gateway = conf_.plugins_.ipam_.gateway_;
      if (!ipam_->setIp(node_name, gateway)) { // 为 gateway 预留一个 IP 地址
        throw OHNO_EXCEPT(
            fmt::format("Failed to reserve gateway:{} for node:{}, the gateway had been used",
                        gateway, node_name),
            false);
      }

      // 为节点 root namespace 创建 Linux bridge，它的地址是节点所有 pod 的网关
      auto bridge = std::make_shared<net::Bridge>(netlink);
      bridge->setName(conf_.plugins_.bridge_);
      bridge->addAddr(std::make_shared<net::Addr>(gateway));
      if (!bridge->setStatus(net::LinkStatus::UP)) {
        OHNO_LOG(warn, "Failed to open bridge:{} in root namespace", conf_.plugins_.bridge_);
      }
      auto host = std::make_shared<ipam::Netns>();
      host->addNic(bridge);
      node->addNetns(ipam::HOST, host); // Kubernetes 节点第一个 namespace 就是宿主机的 root

      // 为节点分配子网
      std::string node_subnet{};
      if (!ipam_->allocateSubnet(node_name, conf_.plugins_.ipam_.subnet_,
                                 conf_.plugins_.subnet_prefix_, node_subnet)) {
        throw OHNO_EXCEPT(fmt::format("Failed to allocate subnet for node:{}", node_name), false);
      }

      // 初始化节点
      node->setSubnet(node_subnet);
      node->setName(node_name);
      node->setUnderlayAddr(node_underlay_addr);
      node->setUnderlayDev(node_underlay_dev);

      cluster_->addNode(node_name, node);
    } else {
      throw OHNO_EXCEPT(fmt::format("No this node:{}, but you do not to create?", node_name),
                        false);
    }
  }
  return node;
}

/**
 * @brief 获取 Kubernetes Pod
 *
 * @param node 节点对象
 * @param pod_name Pod 名称
 * @param create Pod 不存在时创建（true）；Pod 不存在也不创建（false）
 * @return std::shared_ptr<ipam::NetnsIf> Pod 对象，不存在返回 nullptr
 */
auto Cni::getKubernetesPod(const std::shared_ptr<ipam::NodeIf> &node, std::string_view pod_name,
                           bool create) -> std::shared_ptr<ipam::NetnsIf> {
  OHNO_ASSERT(node);
  auto pod = node->getNetns(pod_name);
  if (!pod) {
    if (create) {
      pod = std::make_shared<ipam::Netns>();
      node->addNetns(pod_name, pod); // 将创建的 Pod 加入当前节点
    } else {
      throw OHNO_EXCEPT(fmt::format("No this pod:{}, but you do not to create?", pod_name), false);
    }
  }
  return pod;
}

/**
 * @brief 删除 Kubernetes Pod
 *
 * @param node Kubernetes 节点对象
 * @param pod_name Pod 名称
 * @return true 删除成功
 * @return false 删除失败
 */
auto Cni::delKubernetesPod(const std::shared_ptr<ipam::NodeIf> &node, std::string_view pod_name)
    -> void {
  OHNO_ASSERT(node);
  node->delNetns(pod_name);
}

/**
 * @brief 将 NIC 插入节点 root namespace bridge
 *
 * @param node_name 节点名称
 * @param nic_name 网卡名称
 */
auto Cni::nicPluginBridge(std::string_view node_name, std::string_view nic_name) -> void {
  auto node = getKubernetesNode(node_name);
  if (!node) {
    throw OHNO_EXCEPT(fmt::format("Failed to get Kubernetes node {}", node_name), false);
  }
  auto host = getKubernetesPod(node, ipam::HOST);
  if (!host) {
    OHNO_ASSERT(false); // 创建 Kubernetes 节点的时候一定会自动创建一个 root namespace
  }
  auto host_if = host->getNic(conf_.plugins_.bridge_);
  if (!host_if) {
    OHNO_ASSERT(false); // 创建 Kubernetes 节点的时候一定会自动创建一个 Linux bridge
  }
  auto bridge_if = std::dynamic_pointer_cast<net::Bridge>(host_if);
  if (!bridge_if) {
    throw OHNO_EXCEPT(fmt::format("Failed to get bridge in host netns on node:{}", node_name),
                      false);
  }
  if (!bridge_if->setMaster(nic_name)) {
    throw OHNO_EXCEPT(fmt::format("Failed to set bridge master of veth on node:{}", node_name),
                      false);
  }
}

/**
 * @brief 配置 Pod 网络
 *
 * @param node_name Kubernetes 节点名称
 * @param nic 网卡对象
 * @param nic_name 网卡名称
 */
auto Cni::configPodNetwork(std::string_view node_name, const std::shared_ptr<ohno::net::NicIf> &nic,
                           std::string_view nic_name) -> void {
  std::string pod_addr{};
  if (!ipam_->allocateIp(node_name, pod_addr)) {
    throw OHNO_EXCEPT(fmt::format("Failed to allocate IP address on node:{}", node_name), false);
  }
  if (!nic->addAddr(std::make_shared<net::Addr>(pod_addr))) {
    throw OHNO_EXCEPT(
        fmt::format("Failed to add IP address:{} to veth on node:{}", pod_addr, node_name), false);
  }
  if (!nic->addRoute(std::make_shared<net::Route>(std::string_view{}, conf_.plugins_.ipam_.gateway_,
                                                  nic_name))) {
    throw OHNO_EXCEPT(
        fmt::format("Failed to add default route:{dest:default, via:{}, dev:{}} to veth on node:{}",
                    conf_.plugins_.ipam_.gateway_, nic_name, node_name),
        false);
  }
}

/**
 * @brief 获取 Kubernetes Pod 网卡
 *
 * @param pod Pod 对象
 * @param nic_name 网卡名称
 * @param netlink Netlink 对象（可以为空，为空时不存在也不创建节点）
 * @param node_name 节点名称（可以为空，为空时不存在也不创建节点）
 * @param veth_peer 网卡对端（可以为空，为空时不存在也不创建节点）
 * @param veth_netns 网卡所在网络空间（可以为空，为空时不存在也不创建节点）
 * @return std::shared_ptr<net::NicIf> 网卡对象，不存在时返回 nullptr
 */
auto Cni::getKubernetesNic(const std::shared_ptr<ipam::NetnsIf> &pod, std::string_view nic_name,
                           const std::shared_ptr<net::NetlinkIf> &netlink,
                           std::string_view node_name, std::string_view veth_peer,
                           std::string_view veth_netns) -> std::shared_ptr<net::NicIf> {
  OHNO_ASSERT(pod);
  auto iface = pod->getNic(nic_name);
  if (!iface) {
    if (netlink) {
      iface = std::make_shared<net::Veth>(netlink, veth_peer);
      if (!iface) {
        throw OHNO_EXCEPT(fmt::format("Failed to create iface pair {}--{}", nic_name, veth_peer),
                          false);
      }
      iface->setName(nic_name);
      iface->setStatus(net::LinkStatus::UP);
      iface->addNetns(veth_netns);

      // 获取节点 Linux bridge，将 veth 宿主机一端插入 bridge
      nicPluginBridge(node_name, veth_peer);

      // 配置 Pod 网络
      configPodNetwork(node_name, iface, nic_name);
    } else {
      throw OHNO_EXCEPT(fmt::format("No this iface:{}, but you do not to create?", nic_name),
                        false);
    }
  }
  return iface;
}

/**
 * @brief 删除 Kubernetes Pod 网卡
 *
 * @param pod Pod 对象
 * @param node_name 节点名称
 * @param nic_name 网卡名
 */
auto Cni::delKubernetesNic(const std::shared_ptr<ipam::NetnsIf> &pod, std::string_view node_name,
                           std::string_view nic_name) -> void {
  auto iface = getKubernetesNic(pod, nic_name);
  if (iface) {
    auto addr_obj = iface->getAddr();
    std::string pod_addr = addr_obj ? addr_obj->getCidr() : std::string{};
    iface->cleanup();
    if (!ipam_->releaseIp(node_name, pod_addr)) {
      OHNO_LOG(warn, "Failed to release pod address:{} to IPAM", pod_addr);
    }
    pod->delNic(iface->getName());
  }
}

} // namespace cni
} // namespace ohno
