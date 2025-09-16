#pragma once

// clang-format off
#include <memory>
#include <string_view>
#include "src/net/nic_if.h"
// clang-format on

namespace ohno {
namespace ipam {

class NetnsIf {
public:
  virtual ~NetnsIf() = default;
  virtual auto setName(std::string_view pod_name) -> void = 0;
  virtual auto getName() const -> std::string = 0;
  virtual auto addNic(std::shared_ptr<net::NicIf> nic) -> void = 0;
  virtual auto delNic(std::string_view nic_name) -> void = 0;
  virtual auto getNic(std::string_view nic_name) const -> std::shared_ptr<net::NicIf> = 0;
};

} // namespace ipam
} // namespace ohno
