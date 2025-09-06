// clang-format off
#include "nic.h"
#include "veth.h"
// clang-format on

namespace ohno {
namespace net {

net::Veth::Veth(const std::shared_ptr<NetlinkIf> &netlink, std::string_view peer_name)
    : Nic{netlink}, peer_name_{peer_name} {
  auto name = Nic::getName();
  if (auto ntl = Nic::netlink_.lock()) {
    if (!ntl->vethCreate(name, peer_name)) {
      OHNO_LOG(error, "Failed to create veth pair {}--{}", name, peer_name);
    }
  } else {
    OHNO_LOG(error, "Failed to get Netlink from Veth {}", name);
  }
}

/**
 * @brief 获取 veth 对端接口名
 *
 * @return std::string 对端接口名
 */
auto Veth::getPeerName() const -> std::string { return peer_name_; }

} // namespace net
} // namespace ohno
