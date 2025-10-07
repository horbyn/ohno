#pragma once

// clang-format off
#include <string>
// clang-format on

namespace ohno {
namespace net {

class FdbIf {
public:
  virtual ~FdbIf() = default;
  virtual auto getMac() const -> std::string = 0;
  virtual auto getUnderlayAddr() const -> std::string = 0;
  virtual auto getDev() const -> std::string = 0;
};

} // namespace net
} // namespace ohno
