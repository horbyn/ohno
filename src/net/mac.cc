// clang-format off
#include "mac.h"
#include <net/if_arp.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
// clang-format on

namespace ohno {
namespace net {

Mac::Mac(std::string_view nic_name) {
  if (setMacNative(nic_name)) {
    setMacStr(mac_native_);
  }
}

/**
 * @brief 获取 MAC 地址
 *
 * @return std::string
 */
auto Mac::getMac() -> std::string { return mac_str_; }

/**
 * @brief 根据网络接口设置 MAC 地址数组
 *
 * @param nic_name 网络接口
 * @return true 成功
 * @return false 失败
 */
auto Mac::setMacNative(std::string_view nic_name) -> bool {
  using namespace boost;

  try {
    asio::io_context io_context{};
    asio::ip::udp::socket socket{io_context};
    socket.open(asio::ip::udp::v4());
    int sockfd = socket.native_handle();

    std::string interface_name{nic_name};
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1) {
      OHNO_LOG(warn, "Failed to get MAC address");
      return false;
    }

    // 检查硬件地址类型是否为 ARPHRD_ETHER（以太网）
    if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
      OHNO_LOG(warn, "Interface {} is not Ethernet", interface_name);
      return false;
    }

    // 复制 MAC 地址（sa_data 前 6 字节为 MAC）
    std::memcpy(mac_native_.data(), ifr.ifr_hwaddr.sa_data, SZ_MACLEN_NATIVE);

    return true;
  } catch (const std::exception &e) {
    OHNO_LOG(warn, "Boost.Asio error: {}", e.what());
    return false;
  }
}

/**
 * @brief 设置 MAC 地址的字符串
 *
 * @param mac MAC 地址数组
 * @return true 成功
 * @return false 失败
 */
auto Mac::setMacStr(MacAddress mac) -> bool {
  char buf[SZ_MACLEN_STRING]{};
  std::snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
                mac[4], mac[5]);
  mac_str_ = std::string{buf};
  return !mac_str_.empty();
}

} // namespace net
} // namespace ohno
