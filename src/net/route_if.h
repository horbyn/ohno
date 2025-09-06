#pragma once

// clang-format off
#include <string>
#include "macro.h"
// clang-format on

namespace ohno {
namespace net {

class RouteIf {
public:
  virtual ~RouteIf() = default;
  virtual auto getDest() const -> std::string = 0;
  virtual auto getVia() const -> std::string = 0;
  virtual auto getDev() const -> std::string = 0;
};

} // namespace net
} // namespace ohno
