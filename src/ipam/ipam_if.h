#pragma once

// clang-format off
#include <string_view>
#include <string>
#include <vector>
// clang-format on

namespace ohno {
namespace ipam {

class IpamIf {
public:
  virtual ~IpamIf() = default;
  virtual auto dump() const -> std::string = 0;
  virtual auto allocateSubnet(std::string_view node_name, std::string_view subnet_pool,
                              int subnet_prefix, std::string &subnet) -> bool = 0;
  virtual auto releaseSubnet(std::string_view node_name, std::string_view subnet) -> bool = 0;
  virtual auto getSubnet(std::string_view node_name, std::string &subnet) -> bool = 0;
  virtual auto allocateIp(std::string_view node_name, std::string &result_ip) -> bool = 0;
  virtual auto setIp(std::string_view node_name, std::string_view ip_to_set) -> bool = 0;
  virtual auto releaseIp(std::string_view node_name, std::string_view ip_to_del) -> bool = 0;
};

} // namespace ipam
} // namespace ohno
