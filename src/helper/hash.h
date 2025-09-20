#pragma once

// clang-format off
#include <random>
#include <string_view>
#include <string>
// clang-format on

namespace ohno {
namespace helper {

auto getShortHash(std::string_view long_name) -> std::string;
template <typename Engine = std::mt19937_64>
auto getUniqueId(size_t length, bool use_hex = false) -> std::string;

} // namespace helper
} // namespace ohno

#include "hash.tpp"
