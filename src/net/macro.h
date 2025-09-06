#pragma once

// clang-format off
#include <string_view>
// clang-format on

namespace ohno {
namespace net {

// 用来表示网段范围（比如 10.0.0.0/16 拥有 2^16 个子网）、子网内主机数量
// IPv4 最大值不超过 32 位，但 IPv6 不了解
using Prefix = uint32_t;

enum class IpVersion : uint8_t { RESERVED, IPv4, IPv6 };
enum class LinkStatus : uint8_t { RESERVED, UP, DOWN };

constexpr Prefix MAX_PREFIX_IPV4{32}; // IPv4 地址的最大前缀长度
constexpr std::string_view IPv4_REGEX{R"(^(\d+)\.(\d+)\.(\d+)\.(\d+)/(\d+)$)"};
constexpr std::string_view IPv6_REGEX{R"(^([\da-fA-F:]+)/(\d+)$)"};

constexpr std::string_view PATH_NAMESPACE{"/var/run/netns"};

} // namespace net
} // namespace ohno
