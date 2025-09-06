// clang-format off
#include "env_std.h"
#include <cstdlib>
#include "src/common/assert.h"
// clang-format on

namespace ohno {
namespace util {

// TODO
// 关于静态检查 [concurrency-mt-unsafe]，参考《getenv() + putenv() + setenv() 的 thread-safe 問題》
// https://blog.gslin.org/archives/2024/11/13/12063/getenv-putenv-setenv-%E7%9A%84-thread-safe-%E5%95%8F%E9%A1%8C/
//
// 在多线程环境下调用 glibc 的环境变量相关函数不是线程安全的，暂时忽略这个问题

/**
 * @brief 获取环境变量
 *
 * @param env 环境变量
 * @return std::string 环境变量的值，无效返回空字符串
 */
auto EnvStd::get(std::string_view env) const noexcept -> std::string {
  OHNO_ASSERT(!env.empty());

  try {
    std::lock_guard<std::mutex> lock(mtx_);
    const char *val = std::getenv(env.data()); // NOLINT(concurrency-mt-unsafe)
    return (val != nullptr) ? std::string{val} : std::string{};
  } catch (...) {
    return std::string{};
  }
}

/**
 * @brief 判断是否存在环境变量
 *
 * @param env 环境变量
 * @return true 存在
 * @return false 不存在
 */
auto EnvStd::exist(std::string_view env) const noexcept -> bool { return !get(env).empty(); }

/**
 * @brief 设置环境变量
 *
 * @param env 环境变量
 * @param value 环境变量的值，可以为空
 * @return true 设置成功
 * @return false 设置失败
 */
auto EnvStd::set(std::string_view env, std::string_view value) const noexcept -> bool {
  OHNO_ASSERT(!env.empty());

  try {
    std::lock_guard<std::mutex> lock(mtx_);
    if (::setenv(env.data(), // NOLINT(concurrency-mt-unsafe)
                 value.data(), 1) == -1) {
      OHNO_LOG(error, "Failed to set env var");
      return false;
    }
    return true;
  } catch (...) {
    return false;
  }
}

/**
 * @brief 还原环境变量
 *
 * @param env 环境变量
 * @return true 还原成功
 * @return false 还原失败，环境变量的值不会被改变
 */
auto EnvStd::unset(std::string_view env) const noexcept -> bool {
  OHNO_ASSERT(!env.empty());

  try {
    std::lock_guard<std::mutex> lock(mtx_);
    if (::unsetenv(env.data()) == -1) { // NOLINT(concurrency-mt-unsafe)
      OHNO_LOG(error, "Failed to unset env var");
      return false;
    }
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace util
} // namespace ohno
