#pragma once

// clang-format off
#include "nic.h"
// clang-format on

namespace ohno {
namespace net {

class Bridge : public Nic {
public:
  explicit Bridge(const std::shared_ptr<NetlinkIf> &netlink);

  auto setMaster(std::string_view nic_name) -> bool;
  auto setNoMaster(std::string_view nic_name) -> bool;

private:
  auto setImpl(std::string_view nic_name, bool master) -> bool;
};

} // namespace net
} // namespace ohno
