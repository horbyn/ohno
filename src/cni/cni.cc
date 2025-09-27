// clang-format off
#include "cni.h"
#include <net/if.h>
#include <iostream>
#include "cni_result.h"
#include "cni_error.h"
#include "storage.h"
#include "spdlog/fmt/fmt.h"
#include "src/backend/center.h"
#include "src/common/assert.h"
#include "src/helper/hash.h"
#include "src/ipam/cluster.h"
#include "src/ipam/macro.h"
#include "src/ipam/netns.h"
#include "src/ipam/node.h"
#include "src/net/addr.h"
#include "src/net/bridge.h"
#include "src/net/route.h"
#include "src/net/subnet.h"
#include "src/net/underlay.hpp"
#include "src/net/veth.h"
#include "src/net/netlink/netlink_ip_cmd.h"
#include "src/util/shell_sync.h"
// clang-format on

namespace ohno {
namespace cni {

Cni::Cni(const std::shared_ptr<net::NetlinkIf> &netlink) : netlink_{netlink} {}

/**
 * @brief CNI 配置初始化
 *
 * @param conf CNI 配置
 */
auto Cni::parseConfig(const CniConfig &conf) -> void {
  // CNI 配置文件
  conf_ = conf;
  if (conf_.type_ == DEFAULT_CONF_PLUGINS_NAME) {
    // 校验 bridge 名称
    if (conf_.bridge_.empty()) {
      throw OHNO_CNIERR(7, "Bridge name is empty");
    }
    if (conf_.bridge_.find(SEPARATOR) != std::string::npos) {
      throw OHNO_CNIERR(7, fmt::format("Bridge name can't contain '{}'", SEPARATOR));
    }

    // IPAM 模式
    ipam_mode_ = conf_.ipam_.mode_;
    if (ipam_mode_ == CniConfigIpam::Mode::RESERVED) {
      throw OHNO_CNIERR(7, "Invalid IPAM mode");
    }
  }
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
 * @brief 持久化初始化
 *
 * @param storage 持久化对象
 * @return true 初始化成功
 * @return false 初始化失败
 */
auto Cni::setStorage(std::unique_ptr<StorageIf> storage) -> bool {
  storage_ = std::move(storage);
  if (!storage_) {
    OHNO_LOG(error, "Failed to set storage");
    return false;
  }
  return true;
}

/**
 * @brief 设置 Center 对象
 *
 * @param center 对象
 * @return true 成功
 * @return false 失败
 */
auto Cni::setCenter(std::unique_ptr<backend::CenterIf> center) -> bool {
  center_ = std::move(center);
  if (!center_) {
    OHNO_LOG(error, "Failed to set center obj");
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
  OHNO_ASSERT(ipam_);
  OHNO_LOG(debug, "CNI ADD parameters: container_id:\"{}\", netns:\"{}\", nic_name:\"{}\"",
           container_id, netns, nic_name);

  auto ipam_start = ipam_->dump();
  auto netlink = std::dynamic_pointer_cast<net::NetlinkIpCmd>(netlink_);
  if (!netlink) {
    throw OHNO_CNIERR(7, "Failed to create netlink interface");
  }

  util::ShellSync shell{};
  if (!getCurrentNodeInfo(&shell)) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO, "Failed to get current node info");
  }
  cluster_ = getKubernetesCluster(netlink);
  OHNO_ASSERT(cluster_);

  // 获取 Kubernetes 节点
  auto node = getKubernetesNode(true, netlink);
  if (!node) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                      fmt::format("Failed to get Kubernetes node:{}", node_name_));
  }

  // 获取 Kubernetes Pod
  // 因为 container_id 是 pause 容器的标识，所以可以用来唯一标识一个 Pod
  auto pod = getKubernetesPod(node, container_id, true);
  if (!pod) {
    throw OHNO_CNIERR(
        cni::CNI_ERRCODE_OHNO,
        fmt::format("Failed to get Kubernetes pod:{} of node:{}", container_id, node_name_));
  }

  // 获取 Pod 网卡（Veth）
  // pod 一端使用 $CNI_IFNAME 名称，宿主机一端使用 veth_$CNI_CONTAINERID 名称
  auto veth_host = fmt::format("veth_{}", helper::getShortHash(container_id));
  auto veth_pod = nic_name;
  std::string pod_addr{};

  auto nic =
      getKubernetesNic(pod, veth_pod, false); // 每个 Pod 只保留一张网卡，因为 pod 网络是共享的
  if (nic) {
    // TODO:
    // 如果 CNI 配置给出的子网和 gateway 与 Pod 已生效的网络配置不一样时，此时怎么做？
    // 所有 Pod 都要删除？但 CNI ADD 只是增加一个 Pod，则已删除的其他 Pod 由 CRI
    // 负责重新向 CNI 发命令创建吗？
    net::Subnet subnet_veth{};
    const auto *addr_obj = nic->getAddr();
    if (addr_obj == nullptr) {
      throw OHNO_CNIERR(7, fmt::format("Failed to get veth address:{}", veth_pod));
    }
    subnet_veth.init(addr_obj->getAddrCidr());
    if (!subnet_veth.isSubnetOf(conf_.ipam_.subnet_)) {
      throw OHNO_CNIERR(
          7, fmt::format("CNI interfaces subnet:{} is not CNI configuration subnet of {}",
                         addr_obj->getAddrCidr(), conf_.ipam_.subnet_));
    }

    // 一个 Pod 只支持一个 NIC，所以网卡名称不存在时直接重命名已存在的网卡
    if (!nic->isExist() && !nic->rename(veth_pod)) {
      throw OHNO_CNIERR(
          7, fmt::format("Failed to rename veth:{} to veth:{}", nic->getName(), veth_pod));
    }
  } else {
    // 这个 Pod 第一次创建网卡
    nic = getKubernetesNic(pod, veth_pod, true, netlink, veth_host, netns);
    if (!nic) {
      throw OHNO_CNIERR(7, fmt::format("Failed to get Kubernetes veth pair on pod:{} of node:{}",
                                       container_id, node_name_));
    }
  }
  auto ipam_end = ipam_->dump();
  OHNO_LOG(debug, "\nIPAM start\n{}IPAM end\n{}", ipam_start, ipam_end);

  // 输出
  const auto *addr_obj =
      nic->getAddr(); // 根据 CNI spec：
                      // https://github.com/containernetworking/cni.dev/blob/main/content/docs/spec.md#add-success,
                      // 一个网卡目前只有一个地址
  pod_addr = addr_obj != nullptr ? addr_obj->getAddrCidr() : std::string{UNKNOWN_ADDR_V4};
  OHNO_ASSERT(gateway_);
  CniResult result{.cniversion_ = conf_.cni_version_,
                   .ips_ = std::vector<CniResultIps>{CniResultIps{.address_ = pod_addr,
                                                                  .gateway_ = gateway_->getAddr()}},
                   .interfaces_ = std::vector<CniResultInterfaces>{
                       CniResultInterfaces{.name_ = veth_pod.data(), .sandbox_ = netns.data()}}};
  return nlohmann::json(result).dump(4);
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
    OHNO_ASSERT(ipam_);

    OHNO_LOG(debug, "CNI DEL parameters: container_id:\"{}\", nic_name:\"{}\"", container_id,
             nic_name);

    auto netlink = std::dynamic_pointer_cast<net::NetlinkIpCmd>(netlink_);
    if (!netlink) {
      throw OHNO_CNIERR(7, "Failed to create netlink interface");
    }

    util::ShellSync shell{};
    if (!getCurrentNodeInfo(&shell)) {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO, "Failed to get current node info");
    }

    cluster_ = getKubernetesCluster(netlink);
    if (!cluster_) {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO, "Failed to get kubernetes cluster");
    }

    // 删除 Pod 网络接口
    auto node = getKubernetesNode(false);
    if (!node) {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                        fmt::format("Failed to get Kubernetes node:{}", node_name_));
    }

    auto pod = getKubernetesPod(node, container_id);
    if (!pod) {
      OHNO_LOG(warn, "CNI DEL: Pod had been deleted");
    } else {
      delKubernetesNic(pod, nic_name);
      delKubernetesPod(node, container_id);
    }

    // 删除节点静态路由
    if (node->getNetnsSize() == 1) {
      // 算上宿主机的 root namespace，数量为 1 说明节点刚才删除了最后一个 pod

      // 删除节点 root namespace bridge
      auto host = getKubernetesPod(node, ipam::HOST);
      if (host) {
        delKubernetesNic(host, conf_.bridge_);
        delKubernetesNic(host, node_underlay_dev_);
        delKubernetesPod(node, ipam::HOST);
      }

      // 归还节点子网
      auto node_subnet = node->getSubnet();
      if (!node_subnet.empty() && !ipam_->releaseSubnet(node_name_, node_subnet)) {
        OHNO_LOG(error, "Failed to release node subnet");
      }
    }
  } catch (const cni::CniError &cni_err) {
    OHNO_LOG(error, "CNI DEL failed:\n{}", nlohmann::json(cni_err).dump());
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
  return nlohmann::json(ver).dump(4);
}

/**
 * @brief 获取当前 Kubernetes 节点信息
 *
 * @param shell Shell 对象
 * @return true 获取成功
 * @return false 获取失败
 */
auto Cni::getCurrentNodeInfo(const util::ShellIf *shell) -> bool {
  OHNO_ASSERT(shell != nullptr);

  auto ret = shell->execute("hostname", node_name_);
  if (!ret || node_name_.empty()) {
    OHNO_LOG(critical, "Failed to get current Kubernetes node name");
    return false;
  }

  ret = shell->execute("ip route show default | awk '/default/ {print $5}'", node_underlay_dev_);
  if (!ret || node_underlay_dev_.empty()) {
    OHNO_LOG(critical, "Failed to get current Kubernetes underlay device");
    return false;
  }

  ret = shell->execute(
      fmt::format("ip addr show {} | grep 'inet ' | awk '{{print $2}}'", node_underlay_dev_),
      node_underlay_addr_);
  if (!ret || node_underlay_addr_.empty()) {
    OHNO_LOG(critical, "Failed to get current Kubernetes underlay address");
    return false;
  }

  return true;
}

/**
 * @brief 根据持久化配置生成网卡对象
 *
 * @param pod Kubernetes Pod
 * @param nic 网卡名称
 * @param netlink Netlink 对象
 * @return std::shared_ptr<net::NicIf> 网卡对象
 */
auto Cni::getStorageNic(std::string_view pod, std::string_view nic,
                        const std::weak_ptr<net::NetlinkIf> &netlink)
    -> std::shared_ptr<net::NicIf> {
  std::shared_ptr<net::NicIf> nic_obj{};
  if (pod == ipam::HOST && nic == node_underlay_dev_) {
    nic_obj.reset(new net::Underlay{}); // underlay 网卡在删除时有一些限制
  } else {
    if (nic == conf_.bridge_) {
      nic_obj.reset(new net::Bridge{});
    } else {
      nic_obj.reset(new net::Nic{});
    }
  }
  nic_obj->setName(nic);
  if (!nic_obj->setup(netlink)) {
    return nullptr;
  }
  return nic_obj;
}

/**
 * @brief 初始化 Kubernetes 节点对象
 *
 * @param node 节点对象
 * @param node_subnet 节点子网
 */
auto Cni::initKubernetesNode(const std::shared_ptr<ipam::NodeIf> &node,
                             std::string_view node_subnet) -> void {
  OHNO_ASSERT(!node_subnet.empty());

  node->setName(node_name_);
  node->setSubnet(node_subnet);
  node->setUnderlayAddr(node_underlay_addr_);
  node->setUnderlayDev(node_underlay_dev_);
}

/**
 * @brief 根据持久化网络配置生成 Kubernetes 集群
 *
 * @param netlink Netlink 对象
 * @return std::unique_ptr<ipam::ClusterIf> Kubernetes 集群
 */
auto Cni::getKubernetesCluster(const std::weak_ptr<net::NetlinkIf> &netlink)
    -> std::unique_ptr<ipam::ClusterIf> {
  OHNO_ASSERT(!node_name_.empty());
  OHNO_ASSERT(!node_underlay_dev_.empty());
  OHNO_ASSERT(!node_underlay_addr_.empty());
  OHNO_ASSERT(storage_);

  auto cluster = std::make_unique<ipam::Cluster>();

  std::string subnet{};
  if (!ipam_->getSubnet(node_name_, subnet)) {
    // 没有当前 Kubernetes 节点的信息
    return cluster;
  }

  auto node = std::make_shared<ipam::Node>();
  initKubernetesNode(node, subnet);

  std::vector<std::string> pod_names = storage_->getAllPods(node_name_);
  for (const auto &pod : pod_names) {
    auto pod_obj = std::make_shared<ipam::Netns>();
    pod_obj->setName(pod);

    auto nics = storage_->getAllNic(node_name_, pod);
    for (const auto &nic : nics) {
      auto nic_obj = getStorageNic(pod, nic, netlink);
      if (nic_obj == nullptr) {
        continue;
      }
      if (pod != ipam::HOST) {
        auto netns = storage_->getNetns(node_name_, pod);
        OHNO_ASSERT(!netns.empty()); // CNI ADD 会保证所有 pod 都保存 namespace
        nic_obj->setNetns(netns);
      }

      // 网卡添加 IP 地址
      auto addrs = storage_->getAllAddrs(node_name_, pod, nic);
      for (const auto &addr : addrs) {
        if (gateway_ == nullptr && nic == conf_.bridge_) {
          gateway_.reset(new net::Addr{addr});
        }
        nic_obj->addAddr(std::make_unique<net::Addr>(addr));
      }

      // 网卡添加路由
      auto routes = storage_->getAllRoutes(node_name_, pod, nic);
      for (const auto &route : routes) {
        nic_obj->addRoute(
            std::make_unique<net::Route>(route->getDest(), route->getVia(), route->getDev()));
      }

      pod_obj->addNic(nic_obj);
    } // end NIC

    node->addNetns(pod, pod_obj);
  } // end Pod

  cluster->addNode(node_name_, node);
  return cluster;
}

/**
 * @brief 获取 Linux bridge
 *
 * @param netlink Netlink 对象
 * @param bridge_addr IP 地址
 * @return std::shared_ptr<net::NicIf> 网卡对象
 */
auto Cni::getBridge(const std::weak_ptr<net::NetlinkIf> &netlink, std::string_view bridge_addr)
    -> std::shared_ptr<net::NicIf> {

  // 为节点 root namespace 创建 Linux bridge，它的地址是节点所有 pod 的网关
  auto bridge = std::make_shared<net::Bridge>();
  if (!storage_->addNic(node_name_, ipam::HOST, conf_.bridge_)) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO, fmt::format("Failed to store bridge:{} on node:{}",
                                                         conf_.bridge_, node_name_));
  }
  bridge->setName(conf_.bridge_);
  if (!bridge->setup(netlink)) {
    throw OHNO_CNIERR(7,
                      fmt::format("Failed to create Kubernetes node:{} bridge object", node_name_));
  }

  if (bridge->addAddr(std::make_unique<net::Addr>(bridge_addr))) {
    if (!storage_->addAddr(node_name_, ipam::HOST, conf_.bridge_,
                           std::make_unique<net::Addr>(bridge_addr))) {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                        fmt::format("Failed to store bridge:{} gateway:{} on node:{}",
                                    conf_.bridge_, bridge_addr, node_name_));
    }
  }
  if (!bridge->setStatus(net::LinkStatus::UP)) {
    OHNO_LOG(warn, "Failed to open bridge:{} in root namespace", conf_.bridge_);
  }
  return bridge;
}

/**
 * @brief 获取 root namespace
 *
 * @param node Kubernetes 节点对象
 * @param bridge Linux bridge 对象
 * @param underlay_nic underlay 网卡对象
 */
auto Cni::getRootPod(const std::shared_ptr<ipam::NodeIf> &node,
                     const std::shared_ptr<net::NicIf> &bridge,
                     const std::shared_ptr<net::NicIf> &underlay_nic) -> void {

  auto host = std::make_shared<ipam::Netns>();
  host->addNic(bridge);
  host->addNic(underlay_nic);
  host->setName(ipam::HOST);
  node->addNetns(ipam::HOST, host); // Kubernetes 节点第一个 namespace 就是宿主机的 root
  if (!storage_->addNetns(node_name_, ipam::HOST, ipam::HOST)) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                      fmt::format("Failed to add root namespace:{}", ipam::HOST));
  }
  if (!storage_->addPod(node_name_, ipam::HOST, ipam::HOST)) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO, fmt::format("Failed to store root namespace"));
  }
}

/**
 * @brief 获取 Kubernetes 节点
 *
 * @param get_and_create 获取失败则创建（true）；仅获取（false）
 * @param netlink Netlink 对象（可以为空，为空时不存在也不创建节点）
 * @return std::shared_ptr<ipam::NodeIf> 节点对象，不存在时返回 nullptr
 */
auto Cni::getKubernetesNode(bool get_and_create, const std::weak_ptr<net::NetlinkIf> &netlink)
    -> std::shared_ptr<ipam::NodeIf> {
  OHNO_ASSERT(!node_name_.empty());
  OHNO_ASSERT(!node_underlay_dev_.empty());
  OHNO_ASSERT(!node_underlay_addr_.empty());
  OHNO_ASSERT(cluster_);
  OHNO_ASSERT(ipam_);
  OHNO_ASSERT(center_);

  auto node = cluster_->getNode(node_name_);
  if (!node && get_and_create) {
    if (netlink.lock()) {
      node.reset(new ipam::Node{});

      // 为节点分配子网
      std::string node_subnet{};
      switch (ipam_mode_) {
      case CniConfigIpam::Mode::vxlan: // VxLAN 所有节点的所有 Pod 都共享同一个子网
        node_subnet = conf_.ipam_.subnet_;
        break;
      default: // host-gw 所有节点的 Pod 都有独立的子网
        if (!ipam_->allocateSubnet(node_name_, center_.get(), node_subnet)) {
          throw OHNO_CNIERR(7, fmt::format("Failed to allocate subnet for node:{}", node_name_));
        }
        break;
      }

      std::string gateway{};
      if (!ipam_->allocateIp(node_name_, gateway)) {
        throw OHNO_CNIERR(
            7, fmt::format("Failed to reserve gateway:{} for node:{}, the gateway had been used",
                           gateway, node_name_));
      }
      gateway_.reset(new net::Addr{gateway});
      auto bridge = getBridge(netlink, gateway);

      // 还要创建一个 underlay 网卡负责添加静态路由
      auto underlay_nic = std::make_shared<net::Underlay>();
      if (!storage_->addNic(node_name_, ipam::HOST, node_underlay_dev_)) {
        throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                          fmt::format("Failed to store underlay dev:{} on node:{}",
                                      node_underlay_dev_, node_name_));
      }
      underlay_nic->setName(node_underlay_dev_);

      getRootPod(node, bridge, underlay_nic);

      initKubernetesNode(node, node_subnet);
      cluster_->addNode(node_name_, node);
    } else {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                        fmt::format("No this node:{}, but you do not to create?", node_name_));
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
      pod.reset(new ipam::Netns{});
      pod->setName(pod_name);
      node->addNetns(pod_name, pod); // 将创建的 Pod 加入当前节点
    } else {
      throw OHNO_CNIERR(7, fmt::format("No this pod:{}, but you do not to create?", pod_name));
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
  OHNO_ASSERT(storage_);
  auto netns_name = storage_->getNetns(node_name_, pod_name);
  if (!storage_->delNetns(node_name_, pod_name)) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                      fmt::format("Failed to add namespace of pod:{}", pod_name));
  }
  if (!netns_name.empty() && !storage_->delPod(node_name_, netns_name)) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                      fmt::format("Failed to add container id:{}", netns_name));
  }
  node->delNetns(pod_name);
}

/**
 * @brief 将 NIC 插入节点 root namespace bridge
 *
 * @param nic_name 网卡名称
 */
auto Cni::nicPluginBridge(std::string_view nic_name) -> void {
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(!node_name_.empty());

  auto node = getKubernetesNode(false);
  if (!node) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                      fmt::format("Failed to get Kubernetes node {}", node_name_));
  }
  auto host = getKubernetesPod(node, ipam::HOST);
  if (!host) {
    OHNO_ASSERT(false); // 创建 Kubernetes 节点的时候一定会自动创建一个 root namespace
  }
  auto host_if = host->getNic(conf_.bridge_);
  if (!host_if) {
    OHNO_ASSERT(false); // 创建 Kubernetes 节点的时候一定会自动创建一个 Linux bridge
  }
  auto bridge_if = std::dynamic_pointer_cast<net::Bridge>(host_if);
  if (!bridge_if) {
    throw OHNO_CNIERR(7, fmt::format("Failed to get bridge in host netns on node:{}", node_name_));
  }
  if (!bridge_if->setMaster(nic_name)) {
    throw OHNO_CNIERR(7, fmt::format("Failed to set bridge master of veth on node:{}", node_name_));
  }
}

/**
 * @brief 配置 Pod 网络
 *
 * @param nic 网卡对象
 * @param nic_name 网卡名称
 * @param container_id 容器 id
 */
auto Cni::configPodNetwork(const std::shared_ptr<net::NicIf> &nic, std::string_view nic_name,
                           std::string_view container_id) -> void {
  OHNO_ASSERT(nic);
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(!container_id.empty());
  OHNO_ASSERT(!node_name_.empty());
  OHNO_ASSERT(ipam_);
  OHNO_ASSERT(storage_);
  OHNO_ASSERT(gateway_);

  std::string pod_addr{};
  if (!ipam_->allocateIp(node_name_, pod_addr)) {
    throw OHNO_CNIERR(7, fmt::format("Failed to allocate IP address on node:{}", node_name_));
  }
  if (!nic->addAddr(std::make_unique<net::Addr>(pod_addr))) {
    throw OHNO_CNIERR(
        7, fmt::format("Failed to add IP address:{} to veth on node:{}", pod_addr, node_name_));
  }
  if (!storage_->addAddr(node_name_, container_id, nic_name,
                         std::make_unique<net::Addr>(pod_addr))) {
    throw OHNO_CNIERR(
        cni::CNI_ERRCODE_OHNO,
        fmt::format("Failed to store nic:{} addr:{} on node:{}", nic_name, pod_addr, node_name_));
  }

  auto route_via = gateway_->getAddr();
  if (!nic->addRoute(std::make_unique<net::Route>(std::string_view{}, route_via, nic_name))) {
    throw OHNO_CNIERR(
        7,
        fmt::format("Failed to add default route:{dest:default, via:{}, dev:{}} to veth on node:{}",
                    route_via, nic_name, node_name_));
  }
  if (!storage_->addRoute(node_name_, container_id, nic_name,
                          std::make_unique<net::Route>(std::string_view{}, route_via, nic_name))) {
    throw OHNO_CNIERR(
        cni::CNI_ERRCODE_OHNO,
        fmt::format("Failed to store nic:{} route:{dest:{}, via:{}, dev:{}} on node:{}", nic_name,
                    std::string_view{}, route_via, nic_name, node_name_));
  }
}

/**
 * @brief 获取 Kubernetes Pod 网卡
 *
 * @param pod Pod 对象
 * @param nic_name 网卡名称
 * @param get_and_create 获取失败则创建（true）；仅获取（false）
 * @param netlink Netlink 对象（可以为空，仅在 get_and_create 为 true 时使用）
 * @param veth_peer 网卡对端（可以为空，仅在 get_and_create 为 true 时使用）
 * @param netns Pod 对应的 namespace（可以为空，仅在 create 为 true 时使用）
 * @return std::shared_ptr<net::NicIf> 网卡对象，不存在时返回 nullptr
 */
auto Cni::getKubernetesNic(const std::shared_ptr<ipam::NetnsIf> &pod, std::string_view nic_name,
                           bool get_and_create, const std::weak_ptr<net::NetlinkIf> &netlink,
                           std::string_view veth_peer, std::string_view netns)
    -> std::shared_ptr<net::NicIf> {
  OHNO_ASSERT(pod);
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(!node_name_.empty());
  OHNO_ASSERT(storage_);

  auto iface = pod->getNic(nic_name);
  if (!iface && get_and_create) {
    if (netlink.lock()) {
      auto container_id = pod->getName();
      OHNO_ASSERT(!container_id.empty()); // 创建 Pod 的时候保证已设置 pod 容器 id

      auto cid = storage_->getPod(node_name_, netns);
      if (!cid.empty() && cid != container_id) {
        throw OHNO_CNIERR(7, fmt::format("CRI error: Pod:{} and Pod:{} belongs to one namespace:{}",
                                         cid, container_id, netns));
      }
      if (!storage_->addPod(node_name_, netns, container_id)) {
        throw OHNO_CNIERR(
            cni::CNI_ERRCODE_OHNO,
            fmt::format("Failed to add container:{} to namespace:{}", container_id, netns));
      }

      OHNO_ASSERT(!netns.empty());
      if (!storage_->addNetns(node_name_, container_id, netns)) {
        throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                          fmt::format("Failed to add namespace:{} to pod:{}", netns, container_id));
      }

      iface.reset(new net::Veth{veth_peer});
      if (!storage_->addNic(node_name_, container_id, nic_name)) {
        throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                          fmt::format("Failed to store nic:{} on node:{}", nic_name, node_name_));
      }
      iface->setName(fmt::format("ohno_{}", helper::getShortHash(helper::getUniqueId(
                                                IFNAMSIZ)))); // 创建 Pod 网卡时先使用一个临时网卡名
      if (!iface->setup(netlink)) {
        throw OHNO_CNIERR(7,
                          fmt::format("Failed to create iface pair {}--{}", nic_name, veth_peer));
      }
      if (!iface->setNetns(netns)) {
        throw OHNO_CNIERR(7, fmt::format("Failed to set iface {} netns", nic_name));
      }
      iface->rename(nic_name); // 创建完成并加入 Pod 之后再改名
      iface->setStatus(net::LinkStatus::UP);

      // 获取节点 Linux bridge，将 veth 宿主机一端插入 bridge
      nicPluginBridge(veth_peer);

      // 配置 Pod 网络
      configPodNetwork(iface, nic_name, container_id);
      pod->addNic(iface);
    } else {
      throw OHNO_CNIERR(7, fmt::format("No this iface:{}, but you do not to create?", nic_name));
    }
  }
  return iface;
}

/**
 * @brief 删除 Kubernetes Pod 网卡
 *
 * @param pod Pod 对象
 * @param nic_name 网卡名
 */
auto Cni::delKubernetesNic(const std::shared_ptr<ipam::NetnsIf> &pod, std::string_view nic_name)
    -> void {
  OHNO_ASSERT(pod);
  OHNO_ASSERT(!nic_name.empty());
  OHNO_ASSERT(!node_name_.empty());
  OHNO_ASSERT(ipam_);
  OHNO_ASSERT(storage_);

  auto iface = getKubernetesNic(pod, nic_name, false);
  if (iface) {
    const auto *addr_obj = iface->getAddr();
    std::string pod_addr = addr_obj != nullptr ? addr_obj->getAddrCidr() : std::string{};
    iface->cleanup();
    auto pod_name = pod->getName();
    OHNO_ASSERT(!pod_name.empty()); // 从持久化还原集群对象的时候保证会设置 pod 名称
    if (!(pod_name == ipam::HOST && iface->getName() == node_underlay_dev_)) {
      if (!ipam_->releaseIp(node_name_, pod_addr)) {
        OHNO_LOG(warn, "Failed to release pod address:{} to IPAM", pod_addr);
      }
    }
    if (!storage_->delAddr(node_name_, pod_name, nic_name)) {
      throw OHNO_CNIERR(
          cni::CNI_ERRCODE_OHNO,
          fmt::format("Failed to delete addr of nic:{} on node:{}", nic_name, node_name_));
    }
    if (!storage_->delRoute(node_name_, pod_name, nic_name)) {
      throw OHNO_CNIERR(
          cni::CNI_ERRCODE_OHNO,
          fmt::format("Failed to delete route of nic:{} on node:{}", nic_name, node_name_));
    }
    if (!storage_->delNic(node_name_, pod_name, nic_name)) {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                        fmt::format("Failed to delete nic:{} on node:{}", nic_name, node_name_));
    }
    pod->delNic(nic_name);
  }
}

} // namespace cni
} // namespace ohno
