#pragma once

// clang-format off
#include "netlink_ip_cmd.h"
#include "src/common/assert.h"
// clang-format on

namespace ohno {
namespace net {

/**
 * @brief 执行 ip 命令并处理错误
 *
 * @param command 要执行的命令
 * @param error_message 错误时的日志消息
 * @param args 错误消息的格式化参数
 * @return true 执行成功
 * @return false 执行失败
 */
template <typename... Args>
auto NetlinkIpCmd::executeCommand(std::string_view command, std::string_view error_message,
                                  Args &&...args) -> bool {
  OHNO_ASSERT(!command.empty());
  OHNO_ASSERT(!error_message.empty());
  OHNO_ASSERT(shell_);

  std::string output{}; // 并不关注输出什么内容
  if (!shell_->execute(command, output)) {
    OHNO_LOG(warn, error_message.data(), std::forward<Args>(args)...);
    return false;
  }
  return true;
}

} // namespace net
} // namespace ohno
