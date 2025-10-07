#pragma once

// clang-format off
#include "nic.h"
// clang-format on

namespace ohno {
namespace net {

class Vxlan : public Nic {
public:
  explicit Vxlan(std::string_view underlay_addr, std::string_view underlay_dev);

  auto setup(std::weak_ptr<NetlinkIf> netlink) -> bool override;

private:
  std::string underlay_addr_;
  std::string underlay_dev_;
};

} // namespace net
} // namespace ohno
