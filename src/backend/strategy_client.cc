// clang-format off
#include "strategy_client.h"
#include <fstream>
#include "spdlog/fmt/fmt.h"
#include "scheduler.h"
#include "center.h"
#include "src/backend/host-gw/host_gw.h"
#include "src/common/assert.h"
#include "src/cni/cni_config.h"
#include "src/common/except.h"
#include "src/etcd/etcd_client_shell.h"
#include "src/ipam/ipam.h"
#include "src/net/nic.h"
#include "src/util/shell_sync.h"
// clang-format on

namespace ohno {
namespace backend {

auto StrategyClient::setNetlink(std::weak_ptr<net::NetlinkIf> netlink) -> bool {
  netlink_ = netlink.lock();
  return netlink_ != nullptr;
}

auto StrategyClient::setBackendInfo(const BackendInfo &bkinfo) -> void { bkinfo_ = bkinfo; }

auto StrategyClient::executeStrategy(std::string_view node_name) -> void {
  std::ifstream ifile{std::string{PATH_CNI_CONF}};
  if (!ifile.is_open()) {
    throw OHNO_EXCEPT(fmt::format("Cannot open CNI configuration:{}", PATH_CNI_CONF), false);
  }

  nlohmann::json json{};
  ifile >> json;
  cni::CniConfig cni_conf = json;

  scheduler_.reset(new Scheduler{});
  OHNO_ASSERT(scheduler_ != nullptr);

  switch (cni_conf.ipam_.mode_) {
  case cni::CniConfigIpam::Mode::host_gw:
    scheduler_->setStrategy(getHostGw());
    break;
  default:
    throw OHNO_EXCEPT("CNI configuration has invalid entry(ipam.mode)", false);
  }

  scheduler_->start(node_name);
}

auto StrategyClient::stopStrategy() -> void {
  if (scheduler_ != nullptr) {
    scheduler_->stop();
  }
}

auto StrategyClient::getHostGw() const -> std::unique_ptr<BackendIf> {
  OHNO_ASSERT(netlink_ != nullptr);
  OHNO_ASSERT(!bkinfo_.api_server_.empty());
  OHNO_ASSERT(bkinfo_.refresh_interval_ != 0);

  auto ipam = std::make_unique<ipam::Ipam>();
  if (!ipam->init(std::make_unique<etcd::EtcdClientShell>(etcd::EtcdData{Center::getEtcdClusters()},
                                                          std::make_unique<util::ShellSync>(),
                                                          std::make_unique<util::EnvStd>()))) {
    throw OHNO_EXCEPT("Failed to initialize IPAM, please check in ETCD cluster", false);
  }
  auto host_gw = std::make_unique<HostGw>();
  host_gw->setInterval(bkinfo_.refresh_interval_);
  auto center = std::make_unique<Center>(bkinfo_.api_server_, bkinfo_.ssl_, Center::Type::POD);
  if (!center->test()) {
    throw OHNO_EXCEPT("Kubernetes api server is unhealthy", false);
  }
  host_gw->setCenter(std::move(center));
  host_gw->setIpam(std::move(ipam));
  if (!host_gw->setNic(std::make_unique<net::Nic>(), netlink_)) {
    throw OHNO_EXCEPT("Cannot set root namespace Netlink", false);
  }
  return host_gw;
}

} // namespace backend
} // namespace ohno
