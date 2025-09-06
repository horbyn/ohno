// clang-format off
#include "bridge.h"
// clang-format on

namespace ohno {
namespace net {

Bridge::Bridge(const std::shared_ptr<NetlinkIf> &netlink) : Nic{netlink} {
  auto name = Nic::getName();
  if (auto ntl = Nic::netlink_.lock()) {
    if (!ntl->bridgeCreate(name)) {
      OHNO_LOG(error, "Failed to create bridge {}", name);
    }
  } else {
    OHNO_LOG(error, "Failed to get Netlink from Bridge {}", name);
  }
}

/**
 * @brief 将网络接口插入 Linux bridge
 *
 * @param nic_name 网络接口名称
 * @return true 插入成功
 * @return false 插入失败
 */
auto Bridge::setMaster(std::string_view nic_name) -> bool { return setImpl(nic_name, true); }

/**
 * @brief 将网络接口从 Linux bridge 中拔出
 *
 * @param nic_name 网络接口名称
 * @return true 拔出成功
 * @return false 拔出失败
 */
auto Bridge::setNoMaster(std::string_view nic_name) -> bool { return setImpl(nic_name, false); }

/**
 * @brief 向 Linux bridge 中插拔网络接口
 *
 * @param nic_name 网络接口名称
 * @param master 插入（true），拔出（false）
 * @return true 操作成功
 * @return false 操作失败
 */
auto Bridge::setImpl(std::string_view nic_name, bool master) -> bool {
  auto bridge_name = Nic::getName();
  if (auto ntl = Nic::netlink_.lock()) {
    if (ntl->bridgeSetStatus(nic_name, master, bridge_name, Nic::getNetns())) {
      return true;
    }

    OHNO_LOG(error, "failed to plug {} into bridge {}", nic_name, bridge_name);
  } else {
    OHNO_LOG(error, "Failed to get Netlink from Bridge of {}", bridge_name);
  }
  return false;
}

} // namespace net
} // namespace ohno
