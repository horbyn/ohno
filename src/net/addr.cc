// clang-format off
#include "addr.h"
#include <regex>
#include "spdlog/fmt/fmt.h"
#include "src/common/except.h"
// clang-format on

namespace ohno {
namespace net {

Addr::Addr(std::string_view cidr) {
  // TODO: 将 Boost 方法调用过程从构造函数中移除出去

  try {
    if (cidr.empty()) {
      throw std::invalid_argument("Missing CIDR");
    }

    const std::regex IPv4_RE(IPv4_REGEX.data());
    std::string cidr_str{cidr};
    std::string addr_str{};

    size_t pos = cidr_str.find('/');
    if (pos == std::string::npos) {
      addr_str = cidr_str;
      prefix_ = MAX_PREFIX_IPV4;
    } else {
      addr_str = cidr_str.substr(0, pos);
      prefix_ = static_cast<Prefix>(std::stoi(cidr_str.substr(pos + 1)));
    }

    if (std::regex_match(cidr_str, IPv4_RE)) {
      // IPv4 解析
      ipversion_ = IpVersion::IPv4;
      address_v4_ = boost::asio::ip::make_address_v4(addr_str);
      OHNO_LOG(trace, "Addr ctor cidr {} belongs to IPv4, with IP {}", cidr_str,
               address_v4_.to_string());
    } else {
      // IPv6 解析
      ipversion_ = IpVersion::IPv6;
      address_v6_ = boost::asio::ip::make_address_v6(addr_str);
      OHNO_LOG(trace, "Addr ctor cidr {} belongs to IPv6, with IP {}", cidr_str,
               address_v6_.to_string());
    }
  } catch (const std::exception &err) {
    OHNO_LOG(warn, "cidr \"{}\" is invalid, {}", cidr, err.what());
    ipversion_ = IpVersion::RESERVED;
  }
}

/**
 * @brief 获取不带 CIDR 格式的 IP 地址
 *
 * @return std::string 字符串
 */
auto Addr::getAddr() const -> std::string {
  checkIpv6();
  return address_v4_.to_string();
}

/**
 * @brief 获取 IP 地址 CIDR 格式
 *
 * @return std::string 字符串
 */
auto Addr::getAddrCidr() const -> std::string {
  return fmt::format("{}/{}", getAddr(), getPrefix());
}

/**
 * @brief 获取 IP 地址子网掩码
 *
 * @return Prefix 子网掩码
 */
auto Addr::getPrefix() const noexcept -> Prefix { return prefix_; }

/**
 * @brief 获取 IP 版本
 *
 * @return IpVersion 版本
 */
auto Addr::ipVersion() -> IpVersion { return ipversion_; }

/**
 * @brief 暂时不支持 IPv6，所以对于 IPv6 地址抛出 ohno::except::Exception
 *
 */
auto Addr::checkIpv6() const -> void {
  if (ipversion_ == IpVersion::IPv6) {
    throw OHNO_EXCEPT("IPv6 not supported yet", false);
  }
}

} // namespace net
} // namespace ohno
