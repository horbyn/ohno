#pragma once

// clang-format off
#include <vector>
#include <string_view>
#include <string>
#include <cstdint>
// clang-format on

namespace ohno {
namespace ipam {

class SubnetIf {
public:
  // 用来表示网段范围（比如 10.0.0.0/16 拥有 2^16 个子网）、子网内主机数量
  // IPv4 最大值不超过 32 位，但 IPv6 不了解
  using Prefix = uint32_t;

  virtual ~SubnetIf() = default;
  virtual auto init(std::string_view cidr) -> void = 0;
  virtual auto getSubnet() const -> std::string = 0;
  virtual auto getPrefix() const -> Prefix = 0;
  virtual auto generateCidr(Prefix new_prefix, Prefix index) -> std::string = 0;
  virtual auto generateIp(Prefix index) -> std::string = 0;
};

} // namespace ipam
} // namespace ohno
