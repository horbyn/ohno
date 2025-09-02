#pragma once

// clang-format off
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/network_v6.hpp>
#include "subnet_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace ipam {

constexpr SubnetIf::Prefix MAX_PREFIX_IPV4{32}; // IPv4 地址的最大前缀长度

class Subnet final : public SubnetIf, public log::Loggable<log::Id::subnet> {
public:
  auto init(std::string_view cidr) -> void override;
  auto getSubnet() const -> std::string override;
  auto getPrefix() const -> Prefix override;
  auto generateCidr(Prefix new_prefix, Prefix index) -> std::string override;
  auto generateIp(Prefix index) -> std::string override;

  auto getMaxHosts() const -> Prefix;
  auto getMaxSubnetsFromCidr(Prefix new_prefix) const -> Prefix;

private:
  auto checkIpv6() const -> void;
  auto generateSubnet(const boost::asio::ip::network_v4 &base_net, Prefix new_prefix,
                      Prefix index = 0) const -> boost::asio::ip::network_v4;
  static auto generateIpImpl(const boost::asio::ip::network_v4 &base_net, Prefix index)
      -> boost::asio::ip::address_v4;

  boost::asio::ip::network_v4 subnet_v4_;
  boost::asio::ip::network_v6 subnet_v6_;
  bool ipv6_;
};

} // namespace ipam
} // namespace ohno
