// clang-format off
#include "logger.h"
#include <filesystem>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "src/common/enum_name.hpp"
// clang-format on

namespace ohno {
namespace log {

static std::mutex g_mutex{};
static std::unordered_map<std::string, std::shared_ptr<log::LoggerType>> g_map{};
static Level g_level{LOGLEVEL_DEFAULT};
static std::string g_logfile{LOGFILE_DEFAULT};

/**
 * @brief 设置日志等级
 *
 * @param level 日志等级
 */
auto LogConfig::setLevel(Level level) -> void {
  level_ = level;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_level = level;
  }
}

/**
 * @brief 设置日志等级
 *
 * @param level_str 日志等级字符串
 */
auto LogConfig::setLevel(std::string_view level_str) -> void {
  OHNO_GLOBAL_LOG(info, "日志等级: {}", level_str);
  auto level_opt = stringEnum<log::Level>(level_str);
  log::Level level = level_opt.has_value() ? level_opt.value() : log::LOGLEVEL_DEFAULT;
  setLevel(level);
}

/**
 * @brief 获取日志等级
 *
 * @return Level 日志等级
 */
auto LogConfig::getLevel() const noexcept -> Level { return level_; }

/**
 * @brief 设置日志文件路径
 *
 * @param file 日志文件路径
 */
auto LogConfig::setLogFile(std::string_view file) -> void {
  if (!file.empty()) {
    OHNO_GLOBAL_LOG(info, "日志文件: {}", file);
  }

  namespace fs = std::filesystem;
  fs::path log_path =
      file.empty() ? fs::path{std::string{log::LOGFILE_DEFAULT}} : fs::path{std::string{file}};
  if (fs::is_directory(log_path)) {
    log_path /= std::string{LOGNAME_DEFAULT} + ".log";
  }

  file_ = log_path.string();
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_logfile = file_;
  }
}

/**
 * @brief 获取全局的日志对象
 *
 * @param log_name 日志模块名称
 * @return std::shared_ptr<log::LoggerType> 全局日志对象
 */
auto Logger::getLogger(std::string_view log_name) -> std::shared_ptr<log::LoggerType> {
  std::string log_name2 = log_name.empty() ? std::string{LOGNAME_DEFAULT} : std::string{log_name};

  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_map.find(log_name2) == g_map.end()) {
    constexpr auto MAX_SIZE = 4 * 1024 * 1024;
    constexpr auto MAX_FILES = 5;
    auto file_sink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(g_logfile, MAX_SIZE, MAX_FILES);

    std::vector<spdlog::sink_ptr> sinks{file_sink};
    auto temp = std::make_shared<spdlog::logger>(log_name2, sinks.begin(), sinks.end());
    temp->set_level(static_cast<spdlog::level::level_enum>(g_level));
    temp->flush_on(static_cast<spdlog::level::level_enum>(g_level));

    g_map[log_name2] = temp;
  }
  return g_map[log_name2];
}

/**
 * @brief 获取日志等级
 *
 * @return Level 日志等级
 */
auto Logger::getLevel() -> Level {
  Level ret{};
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    ret = g_level;
  }
  return ret;
}

} // namespace log
} // namespace ohno
