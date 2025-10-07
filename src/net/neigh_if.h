#pragma once

// clang-format off
#include <string>
// clang-format on

namespace ohno {
namespace net {

class NeighIf {
public:
  virtual ~NeighIf() = default;
  virtual auto getAddr() const -> std::string = 0;
  virtual auto getMac() const -> std::string = 0;
  virtual auto getDev() const -> std::string = 0;
};

} // namespace net
} // namespace ohno
