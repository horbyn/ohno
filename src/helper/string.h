#pragma once

// clang-format off
#include <string_view>
#include <string>
#include <vector>
// clang-format on

namespace ohno {
namespace helper {

auto split(std::string_view str, char delim) -> std::vector<std::string>;

} // namespace helper
} // namespace ohno
