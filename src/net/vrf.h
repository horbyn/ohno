#pragma once

// clang-format off
#include "bridge.h"
// clang-format on

namespace ohno {
namespace net {

constexpr uint32_t VRF_TABLE{1100};

class Vrf : public Bridge {
public:
  explicit Vrf();
  explicit Vrf(uint32_t table);

  auto setup(std::weak_ptr<NetlinkIf> netlink) -> bool override;

private:
  uint32_t table_;
};

} // namespace net
} // namespace ohno
