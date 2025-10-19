// clang-format off
#include "evpn.h"
#include "src/cni/cni_config.h"
#include "src/common/assert.h"
#include "src/common/enum_name.hpp"
#include "src/common/except.h"
#include "src/net/addr.h"
// clang-format on

namespace ohno {
namespace backend {

/**
 * @brief 初始化环境
 *
 * @param node_name 当前 Kubernetes 节点名称
 */
auto Evpn::start(std::string_view node_name) -> void {
  (void)node_name;
  envSetup();
}

/**
 * @brief 删除环境
 *
 */
auto Evpn::stop() -> void { envCleanup(); }

/**
 * @brief 设置 vrf 网卡对象
 *
 * @param vrf 网卡对象
 */
auto Evpn::setVrf(std::unique_ptr<net::Vrf> vrf) -> void { vrf_ = std::move(vrf); }

/**
 * @brief 设置 l3 bridge 对象
 *
 * @param br 网卡对象
 */
auto Evpn::setl3Bridge(std::unique_ptr<net::Bridge> br) -> void { bridge_l3_ = std::move(br); }

/**
 * @brief 设置 l3 bridge 对象
 *
 * @param br 网卡对象
 */
auto Evpn::setl2Bridge(std::unique_ptr<net::Bridge> br) -> void { bridge_l2_ = std::move(br); }

/**
 * @brief 设置 VTEP 对象
 *
 * @param vtep 网卡对象
 */
auto Evpn::setVxlan(std::unique_ptr<net::Vxlan> vtep) -> void { vxlan_ = std::move(vtep); }

/**
 * @brief 初始化环境
 *
 */
auto Evpn::envSetup() const -> void {
  OHNO_ASSERT(vrf_ != nullptr);
  OHNO_ASSERT(!vrf_->getName().empty());
  OHNO_ASSERT(bridge_l3_ != nullptr);
  OHNO_ASSERT(!bridge_l3_->getName().empty());
  OHNO_ASSERT(bridge_l2_ != nullptr);
  OHNO_ASSERT(!bridge_l2_->getName().empty());
  OHNO_ASSERT(vxlan_ != nullptr);
  OHNO_ASSERT(!vxlan_->getName().empty());

  auto vrf_name = vrf_->getName();
  auto bridge_l3_name = bridge_l3_->getName();
  auto bridge_l2_name = bridge_l2_->getName();
  auto vxlan_name = vxlan_->getName();
  if (!vrf_->setStatus(net::LinkStatus::UP)) {
    OHNO_LOG(warn, "Vrf device {} cannot open", vrf_name);
  }
  if (!bridge_l3_->setStatus(net::LinkStatus::UP)) {
    OHNO_LOG(warn, "Bridge device {} cannot open", bridge_l3_name);
  }
  if (!bridge_l2_->setStatus(net::LinkStatus::UP)) {
    OHNO_LOG(warn, "Bridge device {} cannot open", bridge_l2_name);
  }
  if (!vrf_->setMaster(bridge_l3_name, net::BridgeAddrGenMode::none)) {
    OHNO_LOG(warn, "Bridge device {} cannot set master to Vrf device {}", bridge_l3_name, vrf_name);
  }
  if (!vrf_->setMaster(bridge_l2_name, net::BridgeAddrGenMode::none)) {
    OHNO_LOG(warn, "Bridge device {} cannot set master to Vrf device {}", bridge_l2_name, vrf_name);
  }
  if (!vxlan_->setStatus(net::LinkStatus::UP)) {
    OHNO_LOG(warn, "Vxlan device {} cannot open", vxlan_name);
  }
  if (!bridge_l3_->setMaster(vxlan_name, net::BridgeAddrGenMode::reserved)) {
    OHNO_LOG(warn, "Vxlan device {} cannot set master to Bridge device {}", vxlan_name,
             bridge_l3_name);
  }
  if (!vxlan_->setSlave(true, false)) {
    OHNO_LOG(warn, "Vxlan device {} cannot set slave", vxlan_name);
  }
}

/**
 * @brief 清理环境
 *
 */
auto Evpn::envCleanup() const noexcept -> void {
  OHNO_ASSERT(vrf_ != nullptr);
  OHNO_ASSERT(bridge_l3_ != nullptr);
  OHNO_ASSERT(vxlan_ != nullptr);

  try {
    vrf_->cleanup();
    bridge_l3_->cleanup();
    vxlan_->cleanup();

  } catch (const ohno::except::Exception &exc) {
    OHNO_LOG(warn, "Evpn mode cleanup error: {}", exc.getMsg());
  } catch (const std::exception &exc) {
    OHNO_LOG(warn, "Evpn mode cleanup error: {}", exc.what());
  }
}

} // namespace backend
} // namespace ohno
