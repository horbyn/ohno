#pragma once

// clang-format off
#include "nic.h"
// clang-format on

namespace ohno {
namespace net {

class Veth : public Nic {
public:
  explicit Veth(const std::shared_ptr<NetlinkIf> &netlink, std::string_view peer_name);

  auto getPeerName() const -> std::string;

private:
  std::string peer_name_;
};

} // namespace net
} // namespace ohno
