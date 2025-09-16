#pragma once

// clang-format off
#include "nic.h"
// clang-format on

namespace ohno {
namespace net {

class Underlay : public Nic {
public:
  explicit Underlay() { Nic::type_ = Type::SYS; }
};

} // namespace net
} // namespace ohno
