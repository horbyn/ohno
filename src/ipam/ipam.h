#pragma once

// clang-format off
#include <memory>
#include "ipam_if.h"
#include "src/etcd/etcd_client_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace ipam {

constexpr std::string_view ETCD_KEY_PREFIX{"/ohno"};
constexpr std::string_view ETCD_KEY_SUBNET{"/ohno/subnets"};
constexpr std::string_view ETCD_KEY_ADDRESS{"/ohno/addresses"};

class Ipam final : public IpamIf, public log::Loggable<log::Id::ipam> {
public:
  auto init(std::unique_ptr<etcd::EtcdClientIf> etcd_client) -> bool;
  auto dump() const -> std::string override;
  auto allocateSubnet(std::string_view node_name, const backend::CenterIf *center,
                      std::string &subnet) -> bool override;
  auto releaseSubnet(std::string_view node_name, std::string_view subnet) -> bool override;
  auto getSubnet(std::string_view node_name, std::string &subnet) -> bool override;
  auto allocateIp(std::string_view node_name, std::string &result_ip) -> bool override;
  auto releaseIp(std::string_view node_name, std::string_view ip_to_del) -> bool override;

private:
  auto getAllIp(std::string_view node_name, std::vector<std::string> &all_ip) -> bool;
  auto isIpAvailable(std::string_view node_name, std::string_view ip_to_confirm) const -> bool;

  std::unique_ptr<etcd::EtcdClientIf> etcd_client_;
};

} // namespace ipam
} // namespace ohno
