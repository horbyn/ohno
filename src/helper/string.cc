// clang-format off
#include "string.h"
#include <sstream>
// clang-format on

namespace ohno {
namespace helper {

/**
 * @brief 切分字符串
 *
 * @param str 字符串
 * @param delim 分隔符
 * @return std::vector<std::string> 被分割的字符串数组
 */
auto split(std::string_view str, char delim) -> std::vector<std::string> {
  std::vector<std::string> tokens{};
  std::string token{};
  std::istringstream tokenStream(std::string{str});
  while (std::getline(tokenStream, token, delim)) {
    tokens.push_back(token);
  }
  return tokens;
}

} // namespace helper
} // namespace ohno
