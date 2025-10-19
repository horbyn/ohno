#pragma once

// clang-format off
#include <memory>
#include "cni_config.h"
#include "cni_if.h"
#include "storage_if.h"
#include "src/backend/center_if.h"
#include "src/ipam/cluster_if.h"
#include "src/ipam/ipam_if.h"
#include "src/log/logger.h"
#include "src/net/netlink/netlink_if.h"
#include "src/util/shell_if.h"
// clang-format on

namespace ohno {
namespace cni {

constexpr std::string_view UNKNOWN_ADDR_V4{"0.0.0.0/32"};

class Cni final : public CniIf, public log::Loggable<log::Id::cni> {
public:
  explicit Cni(const std::shared_ptr<net::NetlinkIf> &netlink);

  auto parseConfig(const CniConfig &conf) -> void;
  auto parseConfig(const nlohmann::json &json) -> bool;
  auto setCluster(std::unique_ptr<ipam::ClusterIf> cluster) -> bool;
  auto setIpam(std::unique_ptr<ipam::IpamIf> ipam) -> bool;
  auto setStorage(std::unique_ptr<StorageIf> storage) -> bool;
  auto setCenter(std::unique_ptr<backend::CenterIf> center) -> bool;

  auto add(std::string_view container_id, std::string_view netns, std::string_view nic_name)
      -> std::string override;
  auto del(std::string_view container_id, std::string_view nic_name) noexcept -> void override;
  auto version() const -> std::string override;

private:
  auto getStorageNic(std::string_view pod, std::string_view nic,
                     const std::weak_ptr<net::NetlinkIf> &netlink) -> std::shared_ptr<net::NicIf>;
  auto initKubernetesNode(const std::shared_ptr<ipam::NodeIf> &node, std::string_view node_subnet)
      -> void;
  auto getKubernetesCluster(const std::weak_ptr<net::NetlinkIf> &netlink)
      -> std::unique_ptr<ipam::ClusterIf>;
  auto getBridge(const std::weak_ptr<net::NetlinkIf> &netlink, std::string_view bridge_addr)
      -> std::shared_ptr<net::NicIf>;
  auto getVxlan(const std::weak_ptr<net::NetlinkIf> &netlink, std::string_view node_subnet)
      -> std::shared_ptr<net::NicIf>;
  auto getRootPod(const std::shared_ptr<ipam::NodeIf> &node,
                  const std::shared_ptr<net::NicIf> &bridge,
                  const std::shared_ptr<net::NicIf> &vxlan,
                  const std::shared_ptr<net::NicIf> &underlay_nic) -> void;
  auto getKubernetesNode(bool get_and_create, const std::weak_ptr<net::NetlinkIf> &netlink = {})
      -> std::shared_ptr<ipam::NodeIf>;
  static auto getKubernetesPod(const std::shared_ptr<ipam::NodeIf> &node, std::string_view pod_name,
                               bool create = false) -> std::shared_ptr<ipam::NetnsIf>;
  auto delKubernetesPod(const std::shared_ptr<ipam::NodeIf> &node, std::string_view pod_name)
      -> void;
  auto nicPluginBridge(std::string_view nic_name) -> void;
  auto configPodNetwork(const std::shared_ptr<net::NicIf> &nic, std::string_view nic_name,
                        std::string_view container_id) -> void;
  auto getKubernetesNic(const std::shared_ptr<ipam::NetnsIf> &pod, std::string_view nic_name,
                        bool get_and_create, const std::weak_ptr<net::NetlinkIf> &netlink = {},
                        std::string_view veth_peer = {}, std::string_view netns = {})
      -> std::shared_ptr<net::NicIf>;
  auto delKubernetesNic(const std::shared_ptr<ipam::NetnsIf> &pod, std::string_view nic_name)
      -> void;

  std::shared_ptr<net::NetlinkIf> netlink_;
  CniConfig conf_;
  std::unique_ptr<ipam::ClusterIf> cluster_;
  std::unique_ptr<ipam::IpamIf> ipam_;
  std::unique_ptr<StorageIf> storage_;
  std::string node_name_;
  std::string node_underlay_dev_;
  std::string node_underlay_addr_;
  std::unique_ptr<net::AddrIf> gateway_;
  CniConfigIpam::Mode ipam_mode_;
  std::unique_ptr<backend::CenterIf> center_;
};

} // namespace cni
} // namespace ohno
