#pragma once

// clang-format off
#include <string>
#include <string_view>
// clang-format on

namespace ohno {
namespace util {

class EnvIf {
public:
  virtual ~EnvIf() = default;
  virtual auto get(std::string_view env) const noexcept -> std::string = 0;
  virtual auto exist(std::string_view env) const noexcept -> bool = 0;
  virtual auto set(std::string_view env, std::string_view value) const noexcept -> bool = 0;
  virtual auto unset(std::string_view env) const noexcept -> bool = 0;
};

} // namespace util
} // namespace ohno
