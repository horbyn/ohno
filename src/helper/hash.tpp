#pragma once

// clang-format off
#include "hash.h"
#include <iomanip>
#include <sstream>
// clang-format on

namespace ohno {
namespace helper {

constexpr size_t HASH_MASK{0xFF};
constexpr size_t BASE_LENGTH{62};

/**
 * @brief 生成唯一标识
 *
 * @tparam Engine 随机数引擎
 * @param length 标识长度，以字节为单位
 * @param use_hex 转换为十六进制（true）
 * @return std::string 唯一标识
 */
template <typename Engine>
auto getUniqueId(size_t length, bool use_hex) -> std::string {
  thread_local Engine engine(std::random_device{}());
  thread_local std::uniform_int_distribution<uint64_t> dist;

  std::stringstream sstream;
  for (size_t i = 0; i < length; ++i) {
    uint64_t val = dist(engine);
    if (use_hex) {
      sstream << std::hex << (val & HASH_MASK);
    } else {
      const char base62[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
      sstream << base62[val % BASE_LENGTH];
    }
  }
  return sstream.str();
}

} // namespace helper
} // namespace ohno
