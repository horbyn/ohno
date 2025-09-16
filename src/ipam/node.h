#pragma once

// clang-format off
#include "node_if.h"
#include <unordered_map>
#include "src/log/logger.h"
#include "src/net/addr_if.h"
#include "src/net/nic_if.h"
#include "src/net/subnet_if.h"
#include "src/net/netlink/netlink_if.h"
// clang-format on

namespace ohno {
namespace ipam {

class Node : public NodeIf, public log::Loggable<log::Id::ipam> {
public:
  auto setName(std::string_view node_name) -> void override;
  auto getName() const -> std::string override;
  auto addNetns(std::string_view netns_name, std::shared_ptr<NetnsIf> netns) -> void override;
  auto delNetns(std::string_view netns_name) -> void override;
  auto getNetns(std::string_view netns_name) const -> std::shared_ptr<NetnsIf> override;
  auto getNetnsSize() const noexcept -> size_t override;
  auto setSubnet(std::string_view subnet) -> void override;
  auto getSubnet() const -> std::string override;
  auto setUnderlayAddr(std::string_view underlay_addr) -> void override;
  auto getUnderlayAddr() const -> std::string override;
  auto setUnderlayDev(std::string_view underlay_dev) -> void override;
  auto getUnderlayDev() const -> std::string override;
  auto onStaticRouteAdd(const std::vector<std::shared_ptr<NodeIf>> &nodes) -> void override;
  auto onStaticRouteDel(const std::vector<std::shared_ptr<NodeIf>> &nodes, std::string_view network,
                        std::string_view via) -> void override;

private:
  std::string name_;
  std::unordered_map<std::string, std::shared_ptr<NetnsIf>> netns_;
  std::unique_ptr<net::SubnetIf> subnet_;
  std::unique_ptr<net::AddrIf> underlay_addr_;
  std::string underlay_dev_;
};

} // namespace ipam
} // namespace ohno
