#pragma once

// clang-format off
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>
#include "subnet_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace net {

class Subnet final : public SubnetIf, public log::Loggable<log::Id::net> {
public:
  auto init(std::string_view cidr) -> void override;
  auto getSubnet() const -> std::string override;
  auto getPrefix() const -> Prefix override;
  auto generateCidr(Prefix new_prefix, Prefix index) -> std::string override;
  auto generateIp(Prefix index) -> std::string override;
  auto isSubnetOf(std::string_view cidr) const -> bool override;

  auto getMaxHosts() const -> Prefix;
  auto getMaxSubnetsFromCidr(Prefix new_prefix) const -> Prefix;

private:
  // TODO: IPv6 支持

  auto checkIpv6() const -> void;
  auto generateSubnet(const boost::asio::ip::network_v4 &base_net, Prefix new_prefix,
                      Prefix index = 0) const -> boost::asio::ip::network_v4;
  static auto generateIpImpl(const boost::asio::ip::network_v4 &base_net, Prefix index)
      -> boost::asio::ip::address_v4;

  boost::asio::ip::network_v4 subnet_v4_;
  boost::asio::ip::network_v6 subnet_v6_;
  IpVersion ipversion_;
};

} // namespace net
} // namespace ohno
