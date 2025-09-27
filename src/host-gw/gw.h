#pragma once

// clang-format off
#include "gw_if.h"
#include <memory>
#include <unordered_map>
#include "src/backend/center_if.h"
#include "src/ipam/ipam_if.h"
#include "src/log/logger.h"
#include "src/net/nic_if.h"
#include "src/net/netlink/netlink_if.h"
// clang-format on

namespace ohno {
namespace hostgw {

class HostGw : public HostGwIf, public log::Loggable<log::Id::hostgw> {
public:
  auto setCurrentNode(std::string_view name) -> void;
  auto getCurrentNode() const -> std::string;
  auto setCenter(std::unique_ptr<backend::CenterIf> center) -> void;
  auto setIpam(std::unique_ptr<ipam::IpamIf> ipam) -> void;
  auto setNic(std::unique_ptr<net::NicIf> nic, std::weak_ptr<net::NetlinkIf> netlink) -> bool;

  auto eventHandler() -> void override;

private:
  std::string current_node_;
  std::unique_ptr<backend::CenterIf> center_;
  std::unique_ptr<ipam::IpamIf> ipam_;
  std::unique_ptr<net::NicIf> nic_;
  std::unordered_map<std::string, backend::NodeInfo> node_cache_;
};

} // namespace hostgw
} // namespace ohno
