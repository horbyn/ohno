// clang-format off
#include "hash.h"
#include <net/if.h>
#include <openssl/sha.h>
// clang-format on

namespace ohno {
namespace helper {

/**
 * @brief 短哈希生成函数 (FNV-1a 32位变体)
 *
 * @param input 输入字符串
 * @return uint32_t 短哈希
 */
static auto shortHash(std::string_view input) -> uint32_t {
  const uint32_t FNV_prime = 16777619U;
  const uint32_t offset_basis = 2166136261U;
  uint32_t hash = offset_basis;

  for (char cha : input) {
    hash ^= static_cast<uint8_t>(cha);
    hash *= FNV_prime;
  }
  return hash;
}

/**
 * @brief 将一个长字符串转换为一个 6 位字符的短哈希
 *
 * @param long_name 长字符串
 * @return std::string 短哈希字符串
 */
auto getShortHash(std::string_view long_name) -> std::string {
  if (long_name.size() <= IFNAMSIZ - 1) {
    return long_name.data();
  }

  auto hash_val = shortHash(long_name);
  static const size_t BUFFER_LENGTH = 7;
  char hex_buf[BUFFER_LENGTH]{0};
  snprintf(hex_buf, sizeof(hex_buf), "%06x", hash_val); // 6 位十六进制

  return std::string{hex_buf};
}

} // namespace helper
// namespace helper
} // namespace ohno
