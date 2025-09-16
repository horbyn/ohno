#pragma once

// clang-format off
#include "nic.h"
// clang-format on

namespace ohno {
namespace net {

class Veth : public Nic {
public:
  explicit Veth(std::string_view peer_name);

  auto setup(std::weak_ptr<NetlinkIf> netlink) -> bool override;
  auto setStatus(LinkStatus status) -> bool override;
  auto getPeerName() const -> std::string;

private:
  std::string peer_name_;
};

} // namespace net
} // namespace ohno
