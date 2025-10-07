// clang-format off
#include "vxlan.h"
#include "src/common/assert.h"
#include "src/net/addr.h"
// clang-format on

namespace ohno {
namespace net {

Vxlan::Vxlan(std::string_view underlay_addr, std::string_view underlay_dev)
    : underlay_addr_{underlay_addr}, underlay_dev_{underlay_dev} {
  try {
    net::Addr addr{underlay_addr_};
    underlay_addr_ = addr.getAddr();
  } catch (const std::exception &exc) {
    OHNO_LOG(warn, "Failed to parse Vxlan underlay address: {}", exc.what());
  }
}

/**
 * @brief 将 Vxlan 网卡设置到系统网络配置中
 *
 * @param netlink Netlink 对象
 * @return true 设置成功
 * @return false 设置失败
 */
auto Vxlan::setup(std::weak_ptr<NetlinkIf> netlink) -> bool {
  OHNO_ASSERT(!underlay_addr_.empty());
  OHNO_ASSERT(!underlay_dev_.empty());

  Nic::setup(netlink);
  auto name = Nic::getName();
  if (auto ntl = Nic::netlink_.lock()) {
    if (ntl->vxlanCreate(name, underlay_addr_, underlay_dev_)) {
      return true;
    }
  }

  OHNO_LOG(warn, "Failed to setup Netlink for Vxlan {}", name);
  return false;
}

} // namespace net
} // namespace ohno
