#pragma once

// clang-format off
#include <string>
#include <string_view>
// clang-format on

namespace ohno {
namespace util {

class ShellIf {
public:
  virtual ~ShellIf() = default;
  virtual auto execute(std::string_view command, std::string &out) const -> bool = 0;
  virtual auto execute(std::string_view command, std::string &out, std::string &err) const
      -> int = 0;
};

} // namespace util
} // namespace ohno
