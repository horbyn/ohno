#pragma once

// clang-format off
#include <string_view>
#include <string>
// clang-format on

namespace ohno {
namespace helper {

auto getShortHash(std::string_view long_name) -> std::string;

} // namespace helper
} // namespace ohno
