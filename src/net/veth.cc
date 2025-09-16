// clang-format off
#include "nic.h"
#include "veth.h"
// clang-format on

namespace ohno {
namespace net {

Veth::Veth(std::string_view peer_name) : peer_name_{peer_name} {}

/**
 * @brief 将 Veth 网卡设置到系统网络配置中
 *
 * @param netlink Netlink 对象
 * @return true 设置成功
 * @return false 设置失败
 */
auto Veth::setup(std::weak_ptr<NetlinkIf> netlink) -> bool {
  Nic::setup(netlink);

  auto name = Nic::getName();
  auto peer_name = getPeerName();
  if (auto ntl = Nic::netlink_.lock()) {
    if (ntl->vethCreate(name, peer_name)) {
      return true;
    }
  }

  OHNO_LOG(warn, "Failed to setup Netlink for Veth {}--{}", name, peer_name);
  return false;
}

/**
 * @brief 启用 / 禁用网卡
 *
 * @note Veth 分两端，自己一端涉及网络空间，peer 一端不涉及网络空间
 *
 * @param status 启用 / 禁用
 * @return true
 * @return false
 */
auto Veth::setStatus(LinkStatus status) -> bool {
  if (auto ntl = Nic::netlink_.lock()) {
    if (ntl->linkSetStatus(getPeerName(), status)) { // peer 一端不涉及网络空间
      if (Nic::setStatus(status)) {                  // 自己一端
        return true;
      }
    }
  }

  return false;
}

/**
 * @brief 获取 veth 对端接口名
 *
 * @return std::string 对端接口名
 */
auto Veth::getPeerName() const -> std::string { return peer_name_; }

} // namespace net
} // namespace ohno
