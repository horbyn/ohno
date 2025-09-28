// clang-format off
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <string_view>
#include "ohno_version.h"
#include "src/backend/center.h"
#include "src/common/except.h"
#include "src/cni/cni.h"
#include "src/cni/cni_config.h"
#include "src/cni/cni_env.h"
#include "src/cni/cni_error.h"
#include "src/cni/storage.h"
#include "src/etcd/etcd_client_shell.h"
#include "src/ipam/ipam.h"
#include "src/log/logger.h"
#include "src/net/netlink/netlink_ip_cmd.h"
#include "src/util/env_std.h"
#include "src/util/shell_sync.h"
// clang-format on

enum class Type : uint8_t { RESERVED, ADD, DEL, VERSION };
static bool g_del{false}; // DEL 操作不能抛出异常

/**
 * @brief 获取空配置
 *
 * @param argc 命令行参数个数
 * @param argv 命令行选项
 * @return true 成功
 * @return false 失败
 */
auto generateEmptyConfigFile(int argc, char **argv) -> bool {
  using namespace ohno;
  if (argc > 1 && std::string(argv[1]) == "--get-conf") {
    OHNO_GLOBAL_LOG(info, "Generate empty config file");
    cni::CniConfig config{};
    std::ofstream fconfig{"ohno.json"};
    fconfig << nlohmann::json(config).dump(2);
    return true;
  }
  return false;
}

/**
 * @brief 从 stdin 解析 CNI 配置
 *
 * @return ohno::cni::CniConfig 配置对象
 */
auto parseCniConfig() -> ohno::cni::CniConfig {
  using namespace ohno;
  cni::CniConfig config{};
  nlohmann::json json{};
  std::cin >> json;
  config = json;
  OHNO_GLOBAL_LOG(info, "Get CNI config:\n {}", nlohmann::json(config).dump(2));
  return config;
}

/**
 * @brief 解析 CNI 环境变量
 *
 * @return cni::CniEnv 环境变量对象
 */
auto parseCniEnv() -> ohno::cni::CniEnv {
  using namespace ohno;
  cni::CniEnv conf_env{};
  auto env = std::make_unique<util::EnvStd>();
  conf_env.command_ = env->get(cni::JKEY_CNI_CE_COMMAND);
  conf_env.container_id_ = env->get(cni::JKEY_CNI_CE_CONTAINERID);
  conf_env.ifname_ = env->get(cni::JKEY_CNI_CE_IFNAME);
  conf_env.netns_ = env->get(cni::JKEY_CNI_CE_NETNS);
  OHNO_GLOBAL_LOG(info, "Get CNI env var:\n {}", nlohmann::json(conf_env).dump(2));
  return conf_env;
}

/**
 * @brief 确定类型
 *
 * @param command CNI 命令
 * @return Type 类型
 */
auto determineType(std::string_view command) -> Type {
  using namespace ohno;
  if (command == "ADD") {
    return Type::ADD;
  }
  if (command == "DEL") {
    return Type::DEL;
  }
  if (command == "CHECK" || command == "STATUS" || command == "GC") {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_NOT_SUPPORTED,
                      fmt::format("\"{}\" is on the road, bro", command));
  }
  if (command == "VERSION") {
    return Type::VERSION;
  }
  return Type::RESERVED;
}

/**
 * @brief 获取 CNI 插件对象
 *
 * @param config CNI 配置
 * @return std::unique_ptr<ohno::cni::Cni> CNI 插件对象
 */
auto getCniPlugin(const ohno::cni::CniConfig &config) -> std::unique_ptr<ohno::cni::Cni> {
  using namespace ohno;
  auto env = std::make_unique<util::EnvStd>();
  auto api_server = backend::Center::getApiServer(backend::Center::Type::HOST, env.get());
  auto center =
      std::make_unique<backend::Center>(api_server, config.ssl_, backend::Center::Type::HOST);
  if (!center->test()) {
    throw OHNO_CNIERR(7, "Kubernetes api server is unhealthy");
  }
  auto etcd_server = backend::Center::getEtcdClusters();
  auto ipam = std::make_unique<ipam::Ipam>();
  if (!ipam->init(std::make_unique<etcd::EtcdClientShell>(
          etcd::EtcdData{etcd_server}, std::make_unique<util::ShellSync>(), std::move(env)))) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                      "Failed to initialize IPAM, please check in ETCD cluster");
  }
  auto storage = std::make_unique<cni::Storage>();
  if (!storage->init(std::make_unique<etcd::EtcdClientShell>(etcd::EtcdData{etcd_server},
                                                             std::make_unique<util::ShellSync>(),
                                                             std::make_unique<util::EnvStd>()))) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO,
                      "Failed to initialize storage, please check in ETCD cluster");
  }

  auto cni = std::make_unique<cni::Cni>(
      std::make_shared<net::NetlinkIpCmd>(std::make_unique<util::ShellSync>()));
  cni->parseConfig(config);
  if (!cni->setIpam(std::move(ipam)) || !cni->setStorage(std::move(storage)) ||
      !cni->setCenter(std::move(center))) {
    throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO, "Failed to set IPAM, Storage or Center");
  }
  return cni;
}

auto main(int argc, char **argv) -> int {
  using namespace ohno;

  try {
    OHNO_GLOBAL_LOG(info, "Launch ohno v{}", OHNO_VERSION);

    // 留个后门用来生成空的配置文件
    if (generateEmptyConfigFile(argc, argv)) {
      return EXIT_SUCCESS;
    }

    // 从 stdin 获取 CNI 配置
    cni::CniConfig config = parseCniConfig();

    // 设置日志
    log::LogConfig log_conf{};
    log_conf.setLogFile(config.log_);
    log_conf.setLevel(config.loglevel_);

    // 从环境变量中获取 CNI 变量
    cni::CniEnv conf_env = parseCniEnv();

    Type type = determineType(conf_env.command_);
    if (type == Type::DEL) {
      g_del = true;
    }

    // 创建 CNI 插件
    auto cni = getCniPlugin(config);
    std::string output{};
    switch (type) {
    case Type::ADD:
      output = cni->add(conf_env.container_id_, conf_env.netns_, conf_env.ifname_);
      OHNO_GLOBAL_LOG(info, "CNI ADD result:\n{}", output);
      std::cout << output << "\n";
      break;
    case Type::DEL:
      cni->del(conf_env.container_id_, conf_env.ifname_);
      break;
    case Type::VERSION:
      output = cni->version();
      OHNO_GLOBAL_LOG(info, "CNI VERSION result:\n{}", output);
      std::cout << output << "\n";
      break;
    default:
      throw OHNO_CNIERR(4, fmt::format("Unknown CNI env var: {}={}", cni::JKEY_CNI_CE_COMMAND,
                                       conf_env.command_));
      break;
    }

    OHNO_GLOBAL_LOG(info, "ohno CNI finished successfully");
  } catch (const ohno::except::Success &suc) {
    return EXIT_SUCCESS;
  } catch (const ohno::cni::CniError &cni_err) {
    std::cerr << nlohmann::json(cni_err).dump(4) << "\n";
    return g_del ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (const ohno::except::Exception &exc) {
    std::cerr << "[error] " << exc.getMsg() << "\n";
    return g_del ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (const std::exception &exc) {
    std::cerr << "[error] " << exc.what() << "\n";
    return g_del ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
