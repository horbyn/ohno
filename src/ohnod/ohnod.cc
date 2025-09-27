// clang-format off
#include <csignal>
#include <cstdlib>
#include <atomic>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include "unistd.h"
#include "spdlog/fmt/fmt.h"
#include "ohno_version.h"
#include "src/backend/backend_info.h"
#include "src/backend/backend.h"
#include "src/backend/center.h"
#include "src/backend/scheduler.h"
#include "src/common/except.h"
#include "src/log/logger.h"
#include "src/net/netlink/netlink_ip_cmd.h"
#include "src/util/env_std.h"
#include "src/util/shell_sync.h"
// clang-format on

/**
 * @brief 命令行参数集合
 *
 */
struct Config {
  ohno::backend::BackendInfo bkinfo_;
  ohno::log::Level log_level_;
};

std::unique_ptr<ohno::backend::SchedulerIf> g_scheduler{};
constexpr size_t DEF_INTERVAL_SEC{5};

/**
 * @brief 信号处理函数
 *
 * @param sig Linux 信号
 */
void handle_signals(int sig) {
  if (sig == SIGTERM || sig == SIGINT) {
    if (g_scheduler) {
      using namespace ohno;
      OHNO_GLOBAL_LOG(info, "Exiting ohnod with resources releasing ...");
      g_scheduler->stop();
    }
  }
}

/**
 * @brief 输出 usage
 *
 * @param programName 程序名
 */
auto printUsage(const char *programName) -> void {
  std::string loglevel{};
  uint8_t loglevel_num = static_cast<uint8_t>(ohno::log::Level::MAXSIZE);
  for (uint8_t num = 0; num < loglevel_num; ++num) {
    loglevel += ohno::enumName(static_cast<ohno::log::Level>(num));
    if (num != loglevel_num - 1) {
      loglevel += ", ";
    }
  }

  // TODO: 后面用专门的 CMD 解析库来做，这里先简单处理
  std::cout << "Usage: " << programName << " [options]" << "\n";
  std::cout << "Options:" << "\n";
  std::cout << "  --loglevel LEVEL   Log Level (" << loglevel << ")\n";
  std::cout << "  --apiserver URL    Kubernetes API server URL" << "\n";
  std::cout << "  --ssl-dir DIR      Dir of CA certificate and token files" << "\n";
  std::cout << "  --insecure         Disable SSL certificate verification" << "\n";
  std::cout << "  --interval SEC     Refresh interval in seconds (default: " << DEF_INTERVAL_SEC
            << ")" << "\n";
  std::cout << "  --help             Show this help message" << "\n";
}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令个数
 * @param argv 命令选项
 * @return Config 命令行参数
 */
auto parseArguments(int argc, char *argv[]) -> Config {
  Config config{};
  using namespace ohno::backend;

  // 默认值
  config.log_level_ = ohno::log::Level::info;
  config.bkinfo_.api_server_ = "";
  config.bkinfo_.ssl_ = true;
  config.bkinfo_.refresh_interval_ = DEF_INTERVAL_SEC;

  // 解析命令行参数
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "--loglevel" && i + 1 < argc) {
      auto loglevel_opt = ohno::stringEnum<ohno::log::Level>(argv[++i]);
      config.log_level_ = loglevel_opt.has_value() ? loglevel_opt.value() : config.log_level_;
    } else if (arg == "--apiserver" && i + 1 < argc) {
      config.bkinfo_.api_server_ = argv[++i];
    } else if (arg == "--insecure") {
      config.bkinfo_.ssl_ = false;
    } else if (arg == "--interval" && i + 1 < argc) {
      config.bkinfo_.refresh_interval_ = std::stoi(argv[++i]);
    } else if (arg == "--help") {
      printUsage(argv[0]);
      exit(0);
    } else {
      std::cerr << "Unknown option: " << arg << std::endl;
      printUsage(argv[0]);
      exit(1);
    }
  }

  // 如果没有提供 api server 地址，尝试使用默认配置
  if (config.bkinfo_.api_server_.empty()) {
    ohno::util::EnvStd env{};
    config.bkinfo_.api_server_ = Center::getApiServer(Center::Type::POD, &env);
  }

  return config;
}

auto main(int argc, char **argv) -> int {
  using namespace ohno;

  try {
    signal(SIGTERM, handle_signals);
    signal(SIGINT, handle_signals);
    auto config = parseArguments(argc, argv);

    // 设置日志
    log::LogConfig log_conf{};
    log_conf.setLevel(config.log_level_);
    log_conf.setStdout(true);

    OHNO_GLOBAL_LOG(info, "Launch ohnod v{}", OHNO_VERSION);
    OHNO_GLOBAL_LOG(info, "API Server:       {}", config.bkinfo_.api_server_);
    OHNO_GLOBAL_LOG(info, "SSL verification: {}", (config.bkinfo_.ssl_ ? "enabled" : "disabled"));
    OHNO_GLOBAL_LOG(info, "Refresh interval: {}", config.bkinfo_.refresh_interval_);

    std::string node_name{};
    auto shell = std::make_unique<util::ShellSync>();
    auto ret = shell->execute("hostname", node_name);
    if (!ret || node_name.empty()) {
      throw OHNO_EXCEPT("Failed to get current Kubernetes node name", false);
    }

    // 启动 daemon
    auto netlink = std::make_shared<net::NetlinkIpCmd>(std::move(shell));
    g_scheduler = std::make_unique<backend::Scheduler>();
    g_scheduler->setStrategy(std::make_unique<backend::Backend>());
    g_scheduler->start(node_name, netlink, config.bkinfo_);

    // 睡眠
    pause();

    OHNO_GLOBAL_LOG(info, "Ohnod Stop!");
  } catch (const ohno::except::Exception &exc) {
    std::cerr << "[error] " << exc.getMsg() << "\n";
    return EXIT_FAILURE;
  } catch (const std::exception &exc) {
    std::cerr << "[error] " << exc.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
