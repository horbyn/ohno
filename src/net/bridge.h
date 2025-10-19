#pragma once

// clang-format off
#include "nic.h"
// clang-format on

namespace ohno {
namespace net {

class Bridge : public Nic {
public:
  auto setup(std::weak_ptr<NetlinkIf> netlink) -> bool override;
  auto setMaster(std::string_view nic_name, BridgeAddrGenMode mode) -> bool;
  auto setNoMaster(std::string_view nic_name, BridgeAddrGenMode mode) -> bool;

private:
  auto setImpl(std::string_view nic_name, bool master, BridgeAddrGenMode mode) -> bool;
};

} // namespace net
} // namespace ohno
