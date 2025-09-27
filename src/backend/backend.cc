// clang-format off
#include "backend.h"
#include <pthread.h>
#include "center.h"
#include "src/common/assert.h"
#include "src/common/except.h"
#include "src/ipam/ipam.h"
#include "src/etcd/etcd_client_shell.h"
#include "src/host-gw/gw.h"
#include "src/net/nic.h"
#include "src/util/shell_sync.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 启动后端线程
 *
 * @param mode 后端模式
 * @param node_name 当前 Kubernetes 节点名称
 * @param netlink Netlink 对象
 * @param backend_info 后端信息
 */
auto Backend::start(cni::CniConfigIpam::Mode mode, std::string_view node_name,
                    std::weak_ptr<net::NetlinkIf> netlink, const BackendInfo &backend_info)
    -> void {
  OHNO_ASSERT(!node_name.empty());

  if (running_) {
    return;
  }
  running_ = true;

  if (mode == cni::CniConfigIpam::Mode::host_gw) {
    auto ipam = std::make_unique<ipam::Ipam>();
    if (!ipam->init(std::make_unique<etcd::EtcdClientShell>(
            etcd::EtcdData{backend::Center::getEtcdClusters()}, std::make_unique<util::ShellSync>(),
            std::make_unique<util::EnvStd>()))) {
      throw OHNO_EXCEPT("Failed to initialize IPAM, please check in ETCD cluster", false);
    }
    auto host_gw = std::make_unique<hostgw::HostGw>();
    host_gw->setCurrentNode(node_name);
    auto center =
        std::make_unique<Center>(backend_info.api_server_, backend_info.ssl_, Center::Type::POD);
    if (!center->test()) {
      throw OHNO_EXCEPT("Kubernetes api server is unhealthy", false);
    }
    host_gw->setCenter(std::move(center));
    host_gw->setIpam(std::move(ipam));
    if (!host_gw->setNic(std::make_unique<net::Nic>(), netlink)) {
      throw OHNO_EXCEPT("Cannot set root namespace Netlink", false);
    }

    monitor_ = std::thread{[&running = running_, hgw = std::move(host_gw),
                            interval = backend_info.refresh_interval_]() {
      try {
        pthread_setname_np(pthread_self(), "host-gw");

        while (running.load()) {
          hgw->eventHandler();
          std::this_thread::sleep_for(std::chrono::seconds(interval));
        }
      } catch (const ohno::except::Exception &exc) {
        std::cerr << "[error] Ohnod worker thread terminated!" << exc.getMsg() << "\n";
      } catch (const std::exception &exc) {
        std::cerr << "[error] Ohnod worker thread terminated!" << exc.what() << "\n";
      }
    }};
  } else if (mode == cni::CniConfigIpam::Mode::vxlan) {
    throw OHNO_EXCEPT("Not support VxLAN mode", false);
  }
}

/**
 * @brief 停止后端线程
 *
 */
auto Backend::stop() -> void {
  running_ = false;
  if (monitor_.joinable()) {
    monitor_.join();
  }
}

} // namespace backend
} // namespace ohno
