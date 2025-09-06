#pragma once

// clang-format off
#include <string>
#include "macro.h"
// clang-format on

namespace ohno {
namespace net {

class AddrIf {
public:
  virtual ~AddrIf() = default;
  virtual auto getCidr() const -> std::string = 0;
  virtual auto getPrefix() const noexcept -> Prefix = 0;
  virtual auto ipVersion() -> IpVersion = 0;
};

} // namespace net
} // namespace ohno
