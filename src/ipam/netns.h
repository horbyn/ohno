#pragma once

// clang-format off
#include "netns_if.h"
#include <string>
#include "src/log/logger.h"
// clang-format on

namespace ohno {
namespace ipam {

class Netns : public NetnsIf, public log::Loggable<log::Id::ipam> {
public:
  auto addNic(std::shared_ptr<net::NicIf> nic) -> void override;
  auto delNic(std::string_view nic_name) -> void override;
  auto getNic(std::string_view nic_name) const -> std::shared_ptr<net::NicIf> override;

private:
  std::unordered_map<std::string, std::shared_ptr<net::NicIf>>
      nic_; // 网络接口名称 -> 网络接口对象（用 std::shared_ptr<T> 是为了接管生命周期）
};

} // namespace ipam
} // namespace ohno
