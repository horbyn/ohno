// clang-format off
#include "subnet.h"
#include <regex>
#include "src/common/except.h"
// clang-format on

namespace ohno {
namespace ipam {

/**
 * @brief 根据一个 CIDR 字符串初始化子网，出错抛出 ohno::except::Exception
 *
 * @param cidr CIDR 字符串
 */
auto Subnet::init(std::string_view cidr) -> void {
  static const std::regex IPv4_RE(R"(^(\d+)\.(\d+)\.(\d+)\.(\d+)/(\d+)$)");
  static const std::regex IPv6_RE(R"(^([\da-fA-F:]+)/(\d+)$)");

  std::string cidr_str{cidr};
  if (std::regex_match(cidr_str, IPv4_RE)) {
    // IPv4 解析
    ipv6_ = false;
    subnet_v4_ = boost::asio::ip::make_network_v4(cidr_str);
    OHNO_LOG(debug, "cidr {} belongs to IPv4, with subnet {}", cidr, subnet_v4_.to_string());
  } else if (std::regex_match(cidr_str, IPv6_RE)) {
    // IPv6 解析
    ipv6_ = true;
    subnet_v6_ = boost::asio::ip::make_network_v6(cidr_str);
    OHNO_LOG(debug, "cidr {} belongs to IPv6, with subnet {}", cidr, subnet_v6_.to_string());
  } else {
    throw OHNO_EXCEPT("Invalid CIDR format", false);
  }
}

/**
 * @brief 获取子网地址
 *
 * @return std::string 子网地址字符串
 */
auto Subnet::getSubnet() const -> std::string {
  checkIpv6();
  return subnet_v4_.to_string();
}

/**
 * @brief 获取子网前缀
 *
 * @return Prefix 子网前缀
 */
auto Subnet::getPrefix() const -> Prefix {
  checkIpv6();
  return subnet_v4_.prefix_length();
}

/**
 * @brief 从原始子网中划分 CIDR 子网，出错抛出 ohno::except::Exception
 *
 * @param new_prefix 新的子网前缀
 * @param index 划分子网依据
 * @return std::string 新的 CIDR 子网
 */
auto Subnet::generateCidr(Prefix new_prefix, Prefix index) -> std::string {
  checkIpv6();
  auto subnet = generateSubnet(subnet_v4_, new_prefix, index);
  return subnet.address().to_string() + "/" + std::to_string(subnet.prefix_length());
}

/**
 * @brief 从当前子网中生成一个 IP 地址，出错抛出 ohno::except::Exception
 *
 * @param index 生成 IP 的依据
 * @return std::string 新的 IP 地址
 */
auto Subnet::generateIp(Prefix index) -> std::string {
  checkIpv6();
  return Subnet::generateIpImpl(subnet_v4_, index).to_string();
}

/**
 * @brief 获取当前子网最大主机数，出错抛出 ohno::except::Exception
 *
 * @return Prefix 最大主机数
 */
auto Subnet::getMaxHosts() const -> Prefix { return getMaxSubnetsFromCidr(MAX_PREFIX_IPV4); }

/**
 * @brief 根据 CIDR 网段前缀计算最大子网数，出错抛出 ohno::except::Exception
 *
 * @param new_prefix 子网前缀
 * @return Prefix 最大子网数
 */
auto Subnet::getMaxSubnetsFromCidr(Prefix new_prefix) const -> Prefix {
  checkIpv6();
  if (new_prefix <= subnet_v4_.prefix_length()) {
    throw OHNO_EXCEPT(fmt::format("New prefix({}) must be larger than current prefix({})",
                                  new_prefix, subnet_v4_.prefix_length()),
                      false);
  }
  return 1 << (new_prefix - subnet_v4_.prefix_length());
}

/**
 * @brief 暂时不支持 IPv6，所以对于 IPv6 地址抛出 ohno::except::Exception
 *
 */
auto Subnet::checkIpv6() const -> void {
  if (ipv6_) {
    throw OHNO_EXCEPT("IPv6 not supported yet", false);
  }
}

/**
 * @brief 生成子网，出错抛出 ohno::except::Exception
 *
 * @param base_net 基础子网
 * @param new_prefix 新的子网前缀
 * @param index 生成子网的依据
 * @return boost::asio::ip::network_v4 对象
 */
auto Subnet::generateSubnet(const boost::asio::ip::network_v4 &base_net, Prefix new_prefix,
                            Prefix index) const -> boost::asio::ip::network_v4 {
  const auto BASE_PREFIX = base_net.prefix_length();

  // 校验 1: 前缀有效性
  if (new_prefix > MAX_PREFIX_IPV4) {
    throw OHNO_EXCEPT("Invalid IPv4 prefix", false);
  }

  // 校验 2: 前缀大小关系
  if (new_prefix < BASE_PREFIX) {
    throw OHNO_EXCEPT("New prefix smaller than base prefix", false);
  }

  // 计算可划分子网数量
  const auto SUBNET_COUNT = getMaxSubnetsFromCidr(new_prefix);
  OHNO_LOG(debug, "the max hosts in cidr {} according new prefix {} is {}", base_net.to_string(),
           new_prefix, SUBNET_COUNT);

  // 校验 3: 地址空间
  if (index >= SUBNET_COUNT) {
    throw OHNO_EXCEPT("Index out of range", false);
  }

  // 计算子网偏移量
  const uint32_t OFFSET = index << (MAX_PREFIX_IPV4 - new_prefix);
  const boost::asio::ip::address_v4 NEW_ADDR(base_net.address().to_uint() + OFFSET);
  OHNO_LOG(debug, "new subnet address according index {} is {}", index, NEW_ADDR.to_string());

  return {NEW_ADDR, static_cast<uint16_t>(new_prefix)};
}

/**
 * @brief 生成 IP 地址，出错抛出 ohno::except::Exception
 *
 * @param base_net 基础子网
 * @param index 生成 IP 的依据
 * @return boost::asio::ip::address_v4 对象
 */
auto Subnet::generateIpImpl(const boost::asio::ip::network_v4 &base_net, Prefix index)
    -> boost::asio::ip::address_v4 {
  auto prefix = base_net.prefix_length();
  auto base_ip = base_net.address();

  // 检查是否单个地址 (前缀长度等于总位数)
  if (static_cast<Prefix>(prefix) == MAX_PREFIX_IPV4) {
    return base_ip;
  }

  return boost::asio::ip::address_v4(base_ip.to_uint() + index);
}

} // namespace ipam
} // namespace ohno
