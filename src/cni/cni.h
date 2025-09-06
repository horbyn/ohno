#pragma once

// clang-format off
#include <memory>
#include "cni_config.h"
#include "cni_if.h"
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
  explicit Cni(std::unique_ptr<net::NetlinkIf> netlink);

  auto parseConfig(const CniConfig &conf) -> void;
  auto parseConfig(const nlohmann::json &json) -> bool;
  auto setCluster(std::unique_ptr<ipam::ClusterIf> cluster) -> bool;
  auto setIpam(std::unique_ptr<ipam::IpamIf> ipam) -> bool;

  auto add(std::string_view container_id, std::string_view netns, std::string_view nic_name)
      -> std::string override;
  auto del(std::string_view container_id, std::string_view nic_name) noexcept -> void override;
  auto version() const -> std::string override;

private:
  auto getCurrentNodeInfo(std::string &node_name, std::string &underlay_dev,
                          std::string &underlay_addr) const -> bool;
  auto getKubernetesNode(std::string_view node_name,
                         const std::shared_ptr<net::NetlinkIf> &netlink = {},
                         std::string_view node_underlay_dev = {},
                         std::string_view node_underlay_addr = {}) -> std::shared_ptr<ipam::NodeIf>;
  static auto getKubernetesPod(const std::shared_ptr<ipam::NodeIf> &node, std::string_view pod_name,
                               bool create = false) -> std::shared_ptr<ipam::NetnsIf>;
  static auto delKubernetesPod(const std::shared_ptr<ipam::NodeIf> &node, std::string_view pod_name)
      -> void;
  auto nicPluginBridge(std::string_view node_name, std::string_view nic_name) -> void;
  auto configPodNetwork(std::string_view node_name, const std::shared_ptr<ohno::net::NicIf> &nic,
                        std::string_view nic_name) -> void;
  auto getKubernetesNic(const std::shared_ptr<ipam::NetnsIf> &pod, std::string_view nic_name,
                        const std::shared_ptr<net::NetlinkIf> &netlink = {},
                        std::string_view node_name = {}, std::string_view veth_peer = {},
                        std::string_view veth_netns = {}) -> std::shared_ptr<net::NicIf>;
  auto delKubernetesNic(const std::shared_ptr<ipam::NetnsIf> &pod, std::string_view node_name,
                        std::string_view nic_name) -> void;

  std::unique_ptr<net::NetlinkIf> netlink_;
  CniConfig conf_;
  std::unique_ptr<ipam::ClusterIf> cluster_;
  std::unique_ptr<ipam::IpamIf> ipam_;
};

} // namespace cni
} // namespace ohno
