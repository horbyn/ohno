#pragma once

// clang-format off
#include <memory>
#include <string>
#include <string_view>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include "addr_if.h"
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace net {

class Addr : public AddrIf, public log::Loggable<log::Id::net> {
public:
  explicit Addr(std::string_view cidr);

  auto getCidr() const -> std::string override;
  auto getPrefix() const noexcept -> Prefix override;
  auto ipVersion() -> IpVersion override;

private:
  // TODO: IPv6 支持

  auto checkIpv6() const -> void;

  boost::asio::ip::address_v4 address_v4_;
  boost::asio::ip::address_v6 address_v6_;
  IpVersion ipversion_;
  Prefix prefix_;
};

} // namespace net
} // namespace ohno
