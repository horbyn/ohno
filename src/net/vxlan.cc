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

/**
 * @brief 设置 VTEP 桥接从接口属性
 *
 * @param neigh_suppress 启用（true）/ 禁用（false）邻居表抑制
 * @param learning 启用（true）/ 禁用（false）地址学习
 * @return true 成功
 * @return false 失败
 */
auto Vxlan::setSlave(bool neigh_suppress, bool learning) const -> bool {
  auto nic_name = Nic::getName();
  if (auto ntl = Nic::netlink_.lock()) {
    if (ntl->vxlanSetSlave(nic_name, neigh_suppress, learning, getNetns())) {
      return true;
    }
  } else {
    OHNO_LOG(warn, "Failed to get Netlink from Bridge of {}", nic_name);
  }
  return false;
}

} // namespace net
} // namespace ohno
