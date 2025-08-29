// clang-format off
#include "except.h"
#include <cstring>
#include <memory>
#include "spdlog/spdlog.h"
// clang-format on

namespace ohno {
namespace except {

/**
 * @brief 构造异常对象
 * @param file: 使用宏 __FILE__
 * @param line: 使用宏 __LINE__
 * @param msg:  提示信息
 * @param is_errno: 是否从 errno 中获取错误信息
 */
Exception::Exception(std::string_view file, int line, std::string_view msg, bool is_errno) {
  err_ = errno;
  msg_ = fmt::format("[{}:{}] {}", file, line, msg);

  if (is_errno && err_ != 0) {
    constexpr int MAXSIZE = 256;
    std::unique_ptr<char[]> err_msg = std::make_unique<char[]>(MAXSIZE);
    if (::strerror_r(err_, err_msg.get(), MAXSIZE) == 0) {
      msg_ = fmt::format("{} ({} -- {})", msg_, err_, err_msg.get());
    }
  }
}

/**
 * @brief 获取出错信息
 *
 * @return std::string 出错信息
 */
auto Exception::getMsg() const -> std::string { return msg_; }

} // namespace except
} // namespace ohno
