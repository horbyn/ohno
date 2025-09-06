#pragma once

// clang-format off
#include <string_view>
// clang-format on

namespace ohno {
namespace ipam {

// 宿主环境和 Pod 环境本质都是 Linux namespace，用 "host" 表示 root namespace
constexpr std::string_view HOST{"host"};

} // namespace ipam
} // namespace ohno
