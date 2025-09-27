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

auto main(int argc, char **argv) -> int {
  using namespace ohno;

  try {
    OHNO_GLOBAL_LOG(info, "Launch ohno v{}", OHNO_VERSION);

    // 留个后门用来生成空的配置文件
    if (argc > 1) {
      if (std::string(argv[1]) == "--get-conf") {
        OHNO_GLOBAL_LOG(info, "Generate empty config file");

        cni::CniConfig config{};
        std::ofstream fconfig{"ohno.json"};
        fconfig << nlohmann::json(config).dump(2);
        fconfig.close();
        return EXIT_SUCCESS;
      }
    }

    // 从 stdin 获取 CNI 配置
    cni::CniConfig config{};
    nlohmann::json json{};
    std::cin >> json;
    config = json;
    OHNO_GLOBAL_LOG(info, "Get CNI config:\n {}", nlohmann::json(config).dump(2));

    // 设置日志
    log::LogConfig log_conf{};
    log_conf.setLogFile(config.log_);
    log_conf.setLevel(config.loglevel_);

    // 从环境变量中获取 CNI 变量
    cni::CniEnv conf_env{};
    auto env = std::make_unique<util::EnvStd>();
    conf_env.command_ = env->get(cni::JKEY_CNI_CE_COMMAND);
    conf_env.container_id_ = env->get(cni::JKEY_CNI_CE_CONTAINERID);
    conf_env.ifname_ = env->get(cni::JKEY_CNI_CE_IFNAME);
    conf_env.netns_ = env->get(cni::JKEY_CNI_CE_NETNS);
    OHNO_GLOBAL_LOG(info, "Get CNI env var:\n {}", nlohmann::json(conf_env).dump(2));

    Type type{};
    if (conf_env.command_ == "ADD") {
      type = Type::ADD;
    } else if (conf_env.command_ == "DEL") {
      type = Type::DEL;
      g_del = true;
    } else if (conf_env.command_ == "CHECK") {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_NOT_SUPPORTED, "\"CHECK\" is on the road, bro");
    } else if (conf_env.command_ == "STATUS") {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_NOT_SUPPORTED, "\"STATUS\" is on the road, bro");
    } else if (conf_env.command_ == "VERSION") {
      type = Type::VERSION;
    } else if (conf_env.command_ == "GC") {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_NOT_SUPPORTED, "\"GC\" is on the road, bro");
    } else {
      type = Type::RESERVED;
    }

    // 创建 CNI 插件
    auto api_server = backend::Center::getApiServer(backend::Center::Type::HOST, env.get());
    auto center = std::make_unique<backend::Center>(api_server, config.ssl_ ? true : false,
                                                    backend::Center::Type::HOST);
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

    cni::Cni cni{std::make_shared<net::NetlinkIpCmd>(std::make_unique<util::ShellSync>())};
    cni.parseConfig(config);
    if (!cni.setIpam(std::move(ipam))) {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO, "Failed to set IPAM");
    }
    if (!cni.setStorage(std::move(storage))) {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO, "Failed to set Storage");
    }
    if (!cni.setCenter(std::move(center))) {
      throw OHNO_CNIERR(cni::CNI_ERRCODE_OHNO, "Failed to set Center");
    }
    std::string output{};
    switch (type) {
    case Type::ADD:
      output = cni.add(conf_env.container_id_, conf_env.netns_, conf_env.ifname_);
      OHNO_GLOBAL_LOG(info, "CNI ADD result:\n{}", output);
      std::cout << output << "\n";
      break;
    case Type::DEL:
      cni.del(conf_env.container_id_, conf_env.ifname_);
      break;
    case Type::VERSION:
      output = cni.version();
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
