#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
// clang-format on

namespace ohno {
namespace etcd {

class EtcdClientIf {
public:
  virtual ~EtcdClientIf() = default;
  virtual auto test() const -> bool = 0;
  virtual auto put(std::string_view key, std::string_view value) const -> bool = 0;
  virtual auto append(std::string_view key, std::string_view value) const -> bool = 0;
  virtual auto get(std::string_view key, std::string &value) const -> bool = 0;
  virtual auto get(std::string_view key, std::unordered_map<std::string, std::string> &value) const
      -> bool = 0;
  virtual auto del(std::string_view key) const -> bool = 0;
  virtual auto del(std::string_view key, std::string_view value) const -> bool = 0;
  virtual auto list(std::string_view key, std::vector<std::string> &results) const -> bool = 0;
  virtual auto dump(std::string_view key) const -> std::string = 0;
};

} // namespace etcd
} // namespace ohno
