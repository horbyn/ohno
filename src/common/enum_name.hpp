#pragma once

// clang-format off
#include <optional>
#include "magic_enum/magic_enum.hpp"
// clang-format on

namespace ohno {

template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr auto enumName(Enum e) noexcept -> std::string_view {
  return magic_enum::enum_name(e);
}

template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr auto stringEnum(std::string_view str) noexcept -> std::optional<Enum> {
  return magic_enum::enum_cast<Enum>(str);
}

} // namespace ohno
