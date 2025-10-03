/**
 * @file logger.h
 * @brief 日志模块是所有模块中第一个初始化的模块，因此除 common 外不应该依赖于项目其他模块
 *
 *        日志模块有两部分：
 *        - 全局日志：通过 OHNO_GLOBAL_LOG() 宏触发，全局函数中使用
 *        - 模块日志：通过 OHNO_LOG() 宏触发，任何继承了 log::Loggable
 *          的模块都可以使用
 * @date 2025-08-28
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

// clang-format off
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include "spdlog/spdlog.h"
// clang-format on

namespace ohno {
namespace log {

using LoggerType = spdlog::logger;
using LogLevel = spdlog::level::level_enum;

enum class Id : std::uint8_t { ohno, backend, cni, etcd, ipam, net, util, MAXSIZE };
enum class Level : std::uint8_t { trace, debug, info, warn, error, critical, off, MAXSIZE };

constexpr std::string_view LOGNAME_DEFAULT{"ohno"};
constexpr Level LOGLEVEL_DEFAULT{Level::trace};
constexpr std::string_view LOGFILE_DEFAULT{"/var/run/log/ohno.log"};

class LogConfig {
public:
  virtual ~LogConfig() = default;

  virtual auto setLevel(Level level) -> void;
  virtual auto setLevel(std::string_view level_str) -> void;
  virtual auto getLevel() const noexcept -> Level;
  virtual auto setLogFile(std::string_view file) -> void;
  virtual auto setStdout(bool set) -> void;

private:
  Level level_{LOGLEVEL_DEFAULT};
  std::string file_{LOGFILE_DEFAULT};
};

class Logger final {
public:
  static auto getLogger(std::string_view log_name) -> std::shared_ptr<log::LoggerType>;
  static auto getLevel() -> Level;
};

template <Id id>
class Loggable : public LogConfig {
public:
  virtual ~Loggable() = default;
  auto getModuleLogger() const -> std::shared_ptr<log::LoggerType>;
};

} // namespace log

#define OHNO_LEVEL(LEVEL) (static_cast<log::LogLevel>(log::Level::LEVEL))
#define OHNO_LOGGER() log::Logger::getLogger(std::string{log::LOGNAME_DEFAULT})
#define OHNO_GLOBAL_LOG_TO(LOGGER, LEVEL, ...)                                                     \
  do {                                                                                             \
    if (static_cast<uint32_t>(log::Level::LEVEL) >=                                                \
        static_cast<uint32_t>(log::Logger::getLevel())) {                                          \
      LOGGER->log(::spdlog::source_loc{__FILE__, __LINE__, __func__}, OHNO_LEVEL(LEVEL),           \
                  __VA_ARGS__);                                                                    \
    }                                                                                              \
  } while (0)
#define OHNO_MODULE_LOGGER() getModuleLogger()
#define OHNO_LOG_TO(LOGGER, LEVEL, ...)                                                            \
  do {                                                                                             \
    if (static_cast<uint32_t>(log::Level::LEVEL) >= static_cast<uint32_t>(getLevel())) {           \
      LOGGER->log(::spdlog::source_loc{__FILE__, __LINE__, __func__}, OHNO_LEVEL(LEVEL),           \
                  __VA_ARGS__);                                                                    \
    }                                                                                              \
  } while (0)

// TODO: 有没有可能将两个日志宏统一起来？

#define OHNO_GLOBAL_LOG(LEVEL, ...) OHNO_GLOBAL_LOG_TO(OHNO_LOGGER(), LEVEL, ##__VA_ARGS__)
#define OHNO_LOG(LEVEL, ...) OHNO_LOG_TO(OHNO_MODULE_LOGGER(), LEVEL, ##__VA_ARGS__)

} // namespace ohno

#include "logger.tpp"
