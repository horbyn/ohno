#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
// clang-format on

namespace ohno {
namespace ipam {

class EtcdClientIf {
public:
  virtual ~EtcdClientIf() = default;
  virtual auto put(std::string_view key, std::string_view value) -> bool = 0;
  virtual auto append(std::string_view key, std::string_view value) -> bool = 0;
  virtual auto get(std::string_view key, std::string &value) -> bool = 0;
  virtual auto del(std::string_view key) -> bool = 0;
  virtual auto del(std::string_view key, std::string_view value) -> bool = 0;
  virtual auto list(std::string_view key, std::vector<std::string> &results) -> bool = 0;
};

} // namespace ipam
} // namespace ohno
