// clang-format off
#include "strategy_client.h"
#include <fstream>
#include "spdlog/fmt/fmt.h"
#include "scheduler.h"
#include "center.h"
#include "src/backend/host_gw.h"
#include "src/backend/evpn.h"
#include "src/backend/vxlan.h"
#include "src/common/assert.h"
#include "src/cni/cni_config.h"
#include "src/cni/storage.h"
#include "src/common/except.h"
#include "src/etcd/etcd_client_shell.h"
#include "src/ipam/ipam.h"
#include "src/net/nic.h"
#include "src/util/shell_sync.h"
#include "src/util/env_std.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 设置 Netlink 对象
 *
 * @param netlink 对象
 * @return true 成功
 * @return false 失败
 */
auto StrategyClient::setNetlink(std::weak_ptr<net::NetlinkIf> netlink) -> bool {
  netlink_ = netlink.lock();
  return netlink_ != nullptr;
}

/**
 * @brief 设置后端信息
 *
 * @param bkinfo 后端信息
 */
auto StrategyClient::setBackendInfo(const BackendInfo &bkinfo) -> void { bkinfo_ = bkinfo; }

/**
 * @brief 执行后端策略
 *
 * @param node_name 节点名称
 */
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

  scheduler_->setStrategy(getStrategy(cni_conf.ipam_.mode_, cni_conf.bridge_));
  scheduler_->start(node_name);
}

/**
 * @brief 停止后端策略
 *
 */
auto StrategyClient::stopStrategy() -> void {
  if (scheduler_ != nullptr) {
    scheduler_->stop();
  }
}

/**
 * @brief 获取策略对象
 *
 * @param mode 路由模式
 * @param l2svi 二层 SVI
 * @return std::unique_ptr<BackendIf> 策略对象
 */
auto StrategyClient::getStrategy(cni::CniConfigIpam::Mode mode, std::string_view l2svi) const
    -> std::unique_ptr<BackendIf> {
  OHNO_ASSERT(netlink_ != nullptr);
  OHNO_ASSERT(!bkinfo_.api_server_.empty());
  OHNO_ASSERT(bkinfo_.refresh_interval_ != 0);

  std::unique_ptr<BackendIf> strategy{};
  switch (mode) {
  case cni::CniConfigIpam::Mode::host_gw:
    strategy = getHostgw();
    OHNO_LOG(info, "Ohond host-gw mode init successfully!");
    break;
  case cni::CniConfigIpam::Mode::vxlan:
    strategy = getVxlan();
    OHNO_LOG(info, "Ohond vxlan mode init successfully!");
    break;
  case cni::CniConfigIpam::Mode::evpn:
    strategy = getEvpn(l2svi);
    OHNO_LOG(info, "Ohond evpn mode init successfully!");
    break;
  default:
    throw OHNO_EXCEPT("CNI configuration has invalid entry(ipam.mode)", false);
  }

  auto center = std::make_unique<Center>(bkinfo_.api_server_, bkinfo_.ssl_, Center::Type::POD);
  if (!center->test()) {
    throw OHNO_EXCEPT("Kubernetes api server is unhealthy", false);
  }
  auto nic = std::make_unique<net::Nic>();
  if (!nic->setup(netlink_)) {
    throw OHNO_EXCEPT("Failed to setup nic", false);
  }
  strategy->setInterval(bkinfo_.refresh_interval_);
  strategy->setCenter(std::move(center));
  strategy->setNic(std::move(nic));

  return strategy;
}

/**
 * @brief 获取 host-gw 对象
 *
 * @return std::unique_ptr<BackendIf> 后端策略
 */
auto StrategyClient::getHostgw() const -> std::unique_ptr<BackendIf> {
  auto ipam = std::make_unique<ipam::Ipam>();
  if (!ipam->init(std::make_unique<etcd::EtcdClientShell>(etcd::EtcdData{Center::getEtcdClusters()},
                                                          std::make_unique<util::ShellSync>(),
                                                          std::make_unique<util::EnvStd>()))) {
    throw OHNO_EXCEPT("Failed to initialize IPAM, please check in ETCD cluster", false);
  }

  auto hostgw = std::make_unique<HostGw>();
  hostgw->setIpam(std::move(ipam));
  return hostgw;
}

/**
 * @brief 获取 vxlan 对象
 *
 * @return std::unique_ptr<BackendIf> 后端策略
 */
auto StrategyClient::getVxlan() const -> std::unique_ptr<BackendIf> {
  auto storage = std::make_unique<cni::Storage>();
  if (!storage->init(std::make_unique<etcd::EtcdClientShell>(
          etcd::EtcdData{Center::getEtcdClusters()}, std::make_unique<util::ShellSync>(),
          std::make_unique<util::EnvStd>()))) {
    throw OHNO_EXCEPT("Failed to initialize storage, please check in ETCD cluster", false);
  }
  auto vxlan = std::make_unique<Vxlan>();
  vxlan->setStorage(std::move(storage));
  return vxlan;
}

/**
 * @brief 获取 BGP EVPN 对象
 *
 * @param l2svi 二层 SVI
 * @return std::unique_ptr<BackendIf> 后端策略
 */
auto StrategyClient::getEvpn(std::string_view l2svi) const -> std::unique_ptr<BackendIf> {
  OHNO_ASSERT(!l2svi.empty());
  auto evpn = std::make_unique<Evpn>();

  auto vrf = std::make_unique<net::Vrf>();
  vrf->setName(BGP_EVPN_VRF);
  if (!vrf->setup(netlink_)) {
    throw OHNO_EXCEPT("Create Vrf device meets error: invalid Netlink", false);
  }

  auto bridge_l3 = std::make_unique<net::Bridge>();
  bridge_l3->setName(BGP_EVPN_BR);
  if (!bridge_l3->setup(netlink_)) {
    throw OHNO_EXCEPT("Create L3 Bridge device meets error: invalid Netlink", false);
  }

  auto bridge_l2 = std::make_unique<net::Bridge>();
  bridge_l2->setName(l2svi);
  if (!bridge_l2->setup(netlink_)) {
    throw OHNO_EXCEPT("Create L2 Bridge device meets error: invalid Netlink", false);
  }

  util::ShellSync shell{};
  std::string node_name{}, underlay_dev{}, underlay_addr{};
  if (!Center::getNodeInfo(&shell, node_name, underlay_dev, underlay_addr)) {
    throw OHNO_EXCEPT("Cannot get current Kubernetes node infos", false);
  }
  auto vxlan = std::make_unique<net::Vxlan>(underlay_addr, underlay_dev);
  vxlan->setName(BGP_EVPN_VTEP);
  if (!vxlan->setup(netlink_)) {
    throw OHNO_EXCEPT("Create Vxlan device meets error: invalid Netlink", false);
  }

  evpn->setVrf(std::move(vrf));
  evpn->setl3Bridge(std::move(bridge_l3));
  evpn->setl2Bridge(std::move(bridge_l2));
  evpn->setVxlan(std::move(vxlan));
  return evpn;
}

} // namespace backend
} // namespace ohno
