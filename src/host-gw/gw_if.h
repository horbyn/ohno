#pragma once

// clang-format off
#include <string>
#include <string_view>
// clang-format on

namespace ohno {
namespace hostgw {

class HostGwIf {
public:
  virtual ~HostGwIf() = default;
  virtual auto eventHandler() -> void = 0;
};

} // namespace hostgw
} // namespace ohno
