#pragma once

// clang-format off
#include "logger.h"
#include <memory>
#include <string>
#include "src/common/enum_name.hpp"
// clang-format on

namespace ohno {
namespace log {

/**
 * @brief 获取模块的日志对象
 *
 * @tparam id 模块 id
 * @return std::shared_ptr<log::LoggerType> 模块日志对象
 */
template <log::Id id>
auto Loggable<id>::getModuleLogger() -> std::shared_ptr<log::LoggerType> {
  return Logger::getLogger(enumName(id));
}

} // namespace log
} // namespace ohno
