// clang-format off
#include "shell_sync.h"
#include <sstream>
#include <system_error>
#include <boost/process.hpp>
#include "spdlog/fmt/fmt.h"
// clang-format on

namespace ohno {
namespace util {

/**
 * @brief 执行 shell 命令
 *
 * @param command shell 命令（如 "ping -c5 10.0.0.1"）
 * @param out 返回值，记录 stdout 的输出，可能为空
 * @param err 返回值，记录 stderr 的输出，可能为空
 *
 * @return shell 命令返回值
 */
auto ShellSync::execute(std::string_view command, std::string &out, std::string &err) -> int {
  int exit_code = 0;
  try {
    if (command.empty()) {
      err = "Command is empty";
      OHNO_LOG(error, err);
      return -1;
    }

    namespace bp = boost::process;
    bp::ipstream output{};
    bp::ipstream error{};

    bp::child chi{std::string{command}, bp::std_out > output, bp::std_err > error,
                  bp::std_in < bp::null};
    chi.wait();
    exit_code = chi.exit_code();

    std::ostringstream result{};
    result << output.rdbuf();
    out = result.str();

    if (exit_code != 0) {
      result.clear();
      result << error.rdbuf();
      err = result.str();
    }

  } catch (const std::system_error &sys_err) {
    exit_code = -1;
    // TODO: shell 命令可能会修改 errno
    OHNO_LOG(error, "Failed to execute \"{}\": {}", command, sys_err.what());
  }
  return exit_code;
}

} // namespace util
} // namespace ohno
