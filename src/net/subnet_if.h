#pragma once

// clang-format off
#include <vector>
#include <string_view>
#include <string>
#include <cstdint>
#include "macro.h"
// clang-format on

namespace ohno {
namespace net {

class SubnetIf {
public:
  virtual ~SubnetIf() = default;
  virtual auto init(std::string_view cidr) -> void = 0;
  virtual auto getSubnet() const -> std::string = 0;
  virtual auto getPrefix() const -> Prefix = 0;
  virtual auto generateCidr(Prefix new_prefix, Prefix index) -> std::string = 0;
  virtual auto generateIp(Prefix index) -> std::string = 0;
  virtual auto isSubnetOf(std::string_view cidr) const -> bool = 0;
};

} // namespace net
} // namespace ohno
