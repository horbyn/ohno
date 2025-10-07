#pragma once

// clang-format off
#include <string>
// clang-format on

namespace ohno {
namespace net {

class MacIf {
public:
  virtual ~MacIf() = default;
  virtual auto getMac() -> std::string = 0;
};

} // namespace net
} // namespace ohno
