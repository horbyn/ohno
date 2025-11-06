// clang-format off
#include "vrf.h"
// clang-format on

namespace ohno {
namespace net {

Vrf::Vrf() : table_{VRF_TABLE} {}

Vrf::Vrf(uint32_t table) : table_{table} {}

/**
 * @brief 将 Vrf 网卡设置到系统网络配置中
 *
 * @param netlink Netlink 对象
 * @return true 设置成功
 * @return false 设置失败
 */
auto Vrf::setup(std::weak_ptr<NetlinkIf> netlink) -> bool {
  Nic::setup(netlink);
  auto name = Nic::getName();
  if (auto ntl = Nic::netlink_.lock()) {
    if (!ntl->linkExist(name)) {
      if (!ntl->vrfCreate(name, table_)) {
        return false;
      }
    }
    return true;
  }

  OHNO_LOG(warn, "Failed to setup Netlink for Vxlan {}", name);
  return false;
}

} // namespace net
} // namespace ohno
